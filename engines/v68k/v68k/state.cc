/*
	state.cc
	--------
*/

#include "v68k/state.hh"

// v68k
#include "v68k/endian.hh"


#pragma exceptions off


namespace v68k
{
	
	processor_state::processor_state( processor_model model, const memory& mem, bkpt_handler bkpt )
	:
		mem( mem ),
		bkpt( bkpt ),
		model( model ),
		condition()
	{
		uint32_t* p   = (uint32_t*)  &regs;
		uint32_t* end = (uint32_t*) (&regs + 1);
		
		for ( ;  p < end;  ++p )
		{
			*p = 0;
		}
	}
	
	void processor_state::prefetch_instruction_word()
	{
		if ( pc() & 1 )
		{
			address_error();
		}
		else if ( !mem.get_instruction_word( pc(), opcode, program_space() ) )
		{
			bus_error();
		}
	}
	
	uint32_t processor_state::read_mem( uint32_t addr, op_size_t size )
	{
		if ( size != byte_sized  &&  badly_aligned_data( addr ) )
		{
			return address_error();
		}
		
		uint32_t result;
		
		bool ok;
		
		switch ( size )
		{
			case byte_sized:
				uint8_t byte;
				
				ok = mem.get_byte( addr, byte, data_space() );
				
				result = byte;
				
				break;
			
			case word_sized:
				uint16_t word;
				
				ok = mem.get_word( addr, word, data_space() );
				
				result = word;
				
				break;
			
			case long_sized:
				ok = mem.get_long( addr, result, data_space() );
				
				break;
			
			default:
				// Not reached
				ok = 0;  // silence uninitialized-variable warning
				break;
		}
		
		if ( !ok )
		{
			return bus_error();
		}
		
		return result;
	}
	
	uint16_t processor_state::get_CCR() const
	{
		const uint16_t ccr = sr.   x <<  4
		                   | sr.nzvc <<  0;
		
		return ccr;
	}
	
	uint16_t processor_state::get_SR() const
	{
		const uint16_t result = sr.ttsm << 12
		                      | sr. iii <<  8
		                      | sr.   x <<  4
		                      | sr.nzvc <<  0;
		
		return result;
	}
	
	void processor_state::set_CCR( uint16_t new_ccr )
	{
		// ...X NZVC  (all processors)
		
		sr.   x = new_ccr >>  4 & 0x1;
		sr.nzvc = new_ccr >>  0 & 0xF;
	}
	
	void processor_state::set_SR( uint16_t new_sr )
	{
		// T.S. .III ...X NZVC  (all processors)
		// .T.M .... .... ....  (68020 and later)
		
		const bool has_extra_bits = model >= mc68020;
		
		const uint16_t sr_mask = 0xA71F | (0x5000 & -has_extra_bits);
		
		new_sr &= sr_mask;
		
		save_sp();
		
		sr.ttsm = new_sr >> 12;
		sr. iii = new_sr >>  8 & 0xF;
		sr.   x = new_sr >>  4 & 0xF;
		sr.nzvc = new_sr >>  0 & 0xF;
		
		load_sp();
	}
	
	bool processor_state::take_exception_format_0( uint16_t vector_offset )
	{
		const uint16_t saved_sr = get_SR();
		
		set_SR( (saved_sr & 0x3FFF) | 0x2000 );  // Clear T1/T0, set S
		
		uint32_t& sp = a(7);
		
		if ( badly_aligned_data( sp ) )
		{
			return address_error();
		}
		
		const uint32_t size = 8;
		
		sp -= size;
		
		const uint32_t format_and_offset = 0 << 12 | vector_offset;
		
		const bool ok = mem.put_word( sp + 0, saved_sr,          supervisor_data_space )
		              & mem.put_long( sp + 2, pc(),              supervisor_data_space )
		              & mem.put_word( sp + 6, format_and_offset, supervisor_data_space );
		
		if ( !ok )
		{
			return bus_error();
		}
		
		if ( badly_aligned_data( regs.vbr ) )
		{
			return address_error();
		}
		
		if ( !mem.get_long( regs.vbr + vector_offset, pc(), supervisor_data_space ) )
		{
			return bus_error();
		}
		
		prefetch_instruction_word();
		
		return true;
	}
	
	bool processor_state::take_exception_format_2( uint16_t vector_offset, uint32_t instruction_address )
	{
		const uint16_t saved_sr = get_SR();
		
		set_SR( (saved_sr & 0x3FFF) | 0x2000 );  // Clear T1/T0, set S
		
		uint32_t& sp = a(7);
		
		if ( badly_aligned_data( sp ) )
		{
			return address_error();
		}
		
		const uint32_t size = 12;
		
		sp -= size;
		
		const uint32_t format_and_offset = 2 << 12 | vector_offset;
		
		const bool ok = mem.put_word( sp + 0, saved_sr,            supervisor_data_space )
		              & mem.put_long( sp + 2, pc(),                supervisor_data_space )
		              & mem.put_word( sp + 6, format_and_offset,   supervisor_data_space )
		              & mem.put_long( sp + 8, instruction_address, supervisor_data_space );
		
		if ( !ok )
		{
			return bus_error();
		}
		
		if ( badly_aligned_data( regs.vbr ) )
		{
			return address_error();
		}
		
		if ( !mem.get_long( regs.vbr + vector_offset, pc(), supervisor_data_space ) )
		{
			return bus_error();
		}
		
		prefetch_instruction_word();
		
		return true;
	}
	
}


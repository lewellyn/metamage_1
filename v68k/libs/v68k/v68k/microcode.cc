/*
	microcode.cc
	------------
*/

#include "v68k/microcode.hh"

// v68k
#include "v68k/conditional.hh"
#include "v68k/macros.hh"
#include "v68k/state.hh"


#pragma exceptions off


namespace v68k
{
	
	#define N( x )  (!!(x) << 3)
	#define Z( x )  (!!(x) << 2)
	#define V( x )  (!!(x) << 1)
	#define C( x )  (!!(x) << 0)
	
	
	#pragma mark Line 0
	
	op_result microcode_ORI_to_CCR( processor_state& s, op_params& pb )
	{
		const uint32_t data = pb.first;
		
		s.set_CCR( s.get_CCR() | data );
		
		return Ok;
	}
	
	op_result microcode_ORI_to_SR( processor_state& s, op_params& pb )
	{
		const uint32_t data = pb.first;
		
		s.set_SR( s.get_SR() | data );
		
		return Ok;
	}
	
	op_result microcode_BCHG( processor_state& s, op_params& pb )
	{
		pb.result = (1 << pb.first) ^ pb.second;
		
		return Ok;
	}
	
	op_result microcode_BCLR( processor_state& s, op_params& pb )
	{
		pb.result = ~(1 << pb.first) & pb.second;
		
		return Ok;
	}
	
	op_result microcode_BSET( processor_state& s, op_params& pb )
	{
		pb.result = (1 << pb.first) | pb.second;
		
		return Ok;
	}
	
	op_result microcode_MOVEP_to( processor_state& s, op_params& pb )
	{
		const uint32_t x = pb.target;
		
		const uint32_t Dx = s.d( x );
		
		uint8_t* p = s.mem.translate( pb.address, (1 << pb.size) - 1, s.data_space(), mem_write );
		
		if ( p == 0 )  // NULL
		{
			return Bus_error;
		}
		
		switch ( (pb.size == long_sized) + 0 )  // clang hates boolean switch conditions
		{
			case true:
				p[0] = Dx >> 24;
				p[2] = Dx >> 16;
				
				p += 4;
				
				// fall through
			
			case false:
				p[0] = Dx >>  8;
				p[2] = Dx >>  0;
		}
		
		return Ok;
	}
	
	op_result microcode_MOVEP_from( processor_state& s, op_params& pb )
	{
		const uint32_t x = pb.target;
		
		uint32_t& Dx = s.d( x );
		
		const uint8_t* p = s.mem.translate( pb.address, (1 << pb.size) - 1, s.data_space(), mem_read );
		
		if ( p == 0 )  // NULL
		{
			return Bus_error;
		}
		
		uint32_t data = pb.size == long_sized ? 0 : Dx & 0xFFFF0000;
		
		switch ( (pb.size == long_sized) + 0 )  // clang hates boolean switch conditions
		{
			case true:
				data |= p[0] << 24;
				data |= p[2] << 16;
				
				p += 4;
				
				// fall through
			
			case false:
				data |= p[0] << 8;
				data |= p[2] << 0;
		}
		
		Dx = data;
		
		return Ok;
	}
	
	op_result microcode_ANDI_to_CCR( processor_state& s, op_params& pb )
	{
		const uint32_t data = pb.first;
		
		s.set_CCR( s.get_CCR() & data );
		
		return Ok;
	}
	
	op_result microcode_ANDI_to_SR( processor_state& s, op_params& pb )
	{
		const uint32_t data = pb.first;
		
		s.set_SR( s.get_SR() & data );
		
		return Ok;
	}
	
	op_result microcode_EORI_to_CCR( processor_state& s, op_params& pb )
	{
		const uint32_t data = pb.first;
		
		s.set_CCR( s.get_CCR() ^ data );
		
		return Ok;
	}
	
	op_result microcode_EORI_to_SR( processor_state& s, op_params& pb )
	{
		const uint32_t data = pb.first;
		
		s.set_SR( s.get_SR() ^ data );
		
		return Ok;
	}
	
	op_result microcode_MOVES( processor_state& s, op_params& pb )
	{
		const uint32_t more = pb.first;
		
		const int size = byte_count( pb.size );  // 1,2,4
		
		const uint16_t reg_id = more >> 12;
		
		const uint16_t writing = more & 0x0800;
		
		const function_code_t fc = function_code_t( s.regs[ writing ? DFC : SFC ] );
		
		const memory_access_t access = writing ? mem_write : mem_read;
		
		uint8_t* p = s.mem.translate( pb.address, size, fc, access );
		
		if ( p == 0 )
		{
			return Bus_error;
		}
		
		if ( writing )
		{
			const uint32_t data = s.regs[ reg_id ];
			
			switch ( pb.size )
			{
				case long_sized:
					*p++ = data >> 24;
					*p++ = data >> 16 & 0xFF;
				case word_sized:
					*p++ = data >>  8 & 0xFF;
				case byte_sized:
					*p++ = data >>  0 & 0xFF;
				
				default:
					break;
			}
		}
		else
		{
			uint32_t& reg = s.regs[ reg_id ];
			
			uint32_t data = 0;
			
			switch ( pb.size )
			{
				case long_sized:
					data |= *p++ << 24;
					data |= *p++ << 16;
				case word_sized:
					data |= *p++ <<  8;
				case byte_sized:
					data |= *p++ <<  0;
				
				default:
					break;
			}
			
			if ( reg_id & 0x8 )
			{
				reg = sign_extend( data, pb.size );
			}
			else
			{
				reg = update( reg, data, pb.size );
			}
		}
		
		return Ok;
	}
	
	#pragma mark -
	#pragma mark Lines 1-3
	
	op_result microcode_MOVE( processor_state& s, op_params& pb )
	{
		pb.result = pb.first;
		
		return Ok;
	}
	
	#pragma mark -
	#pragma mark Line 4
	
	op_result microcode_CHK( processor_state& s, op_params& pb )
	{
		const int32_t bound = pb.first;
		const int32_t value = pb.second;
		
		if ( value < 0 )
		{
			s.sr.nzvc = 0x8;  // set N, others undefined
		}
		else if ( value > bound )
		{
			s.sr.nzvc = 0x0;  // clear N, others undefined
		}
		else
		{
			return Ok;  // within bounds
		}
		
		return CHK_exception;
	}
	
	op_result microcode_LEA( processor_state& s, op_params& pb )
	{
		const uint32_t n = pb.target;
		
		uint32_t& An = s.a(n);
		
		An = pb.address;
		
		return Ok;
	}
	
	op_result microcode_MOVE_from_CCR( processor_state& s, op_params& pb )
	{
		pb.result = s.get_CCR();
		
		return Ok;
	}
	
	op_result microcode_MOVE_from_SR( processor_state& s, op_params& pb )
	{
		pb.result = s.get_SR();
		
		return Ok;
	}
	
	op_result microcode_MOVE_to_CCR( processor_state& s, op_params& pb )
	{
		const uint32_t data = pb.first;
		
		s.set_CCR( data );
		
		return Ok;
	}
	
	op_result microcode_MOVE_to_SR( processor_state& s, op_params& pb )
	{
		const uint32_t data = pb.first;
		
		s.set_SR( data );
		
		return Ok;
	}
	
	op_result microcode_SWAP( processor_state& s, op_params& pb )
	{
		const uint32_t n = pb.target;
		
		const uint32_t data = s.d(n);
		
		pb.result =
		s.d(n)    = data << 16
		          | data >> 16;
		
		return Ok;
	}
	
	op_result microcode_BKPT( processor_state& s, op_params& pb )
	{
		const uint32_t data = pb.target;  // 3-bit breakpoint vector
		
		s.pc() -= 2;
		
		s.opcode = 0x4AFC;  // ILLEGAL
		
		if ( bkpt_handler f = s.bkpt )
		{
			s.opcode = f( s, data );
		}
		
		return Breakpoint;
	}
	
	op_result microcode_PEA( processor_state& s, op_params& pb )
	{
		uint32_t& sp = s.a(7);
		
		if ( s.badly_aligned_data( sp ) )
		{
			return Address_error;
		}
		
		sp -= 4;
		
		if ( !s.mem.put_long( sp, pb.address, s.data_space() ) )
		{
			return Bus_error;
		}
		
		return Ok;
	}
	
	op_result microcode_EXT_W( processor_state& s, op_params& pb )
	{
		const uint32_t n = pb.target;
		
		uint32_t& Dn = s.d(n);
		
		const int8_t byte = Dn;
		
		const int16_t word = byte;
		
		Dn = (Dn & 0xFFFF0000) | word;
		
		pb.result = word;
		
		return Ok;
	}
	
	op_result microcode_EXT_L( processor_state& s, op_params& pb )
	{
		const uint32_t n = pb.target;
		
		uint32_t& Dn = s.d(n);
		
		const int16_t word = Dn;
		
		const int32_t longword = word;
		
		Dn = longword;
		
		pb.result = longword;
		
		return Ok;
	}
	
	op_result microcode_EXTB( processor_state& s, op_params& pb )
	{
		const uint32_t n = pb.target;
		
		uint32_t& Dn = s.d(n);
		
		const int8_t byte = Dn;
		
		const int32_t longword = byte;
		
		Dn = longword;
		
		pb.result = longword;
		
		return Ok;
	}
	
	op_result microcode_TAS( processor_state& s, op_params& pb )
	{
		const int32_t data = sign_extend( pb.second, byte_sized );
		
		s.sr.nzvc = N( data <  0 )
		          | Z( data == 0 )
		          | V( 0 )
		          | C( 0 );
		
		pb.result = data | 0x80;
		
		return Ok;
	}
	
	op_result microcode_MOVEM_to( processor_state& s, op_params& pb )
	{
		uint16_t mask = pb.first;
		uint32_t addr = pb.address;
		
		const uint32_t update_register = pb.target;
		
		const bool longword_sized = pb.size == long_sized;
		
		int32_t increment = 2 << longword_sized;
		
		if ( update_register )
		{
			increment = -increment;
		}
		
		for ( int r = 0;  mask != 0;  ++r, mask >>= 1 )
		{
			if ( mask & 0x1 )
			{
				const uint32_t data = s.regs[ update_register ? 15 - r : r ];
				
				const bool ok = longword_sized ? s.mem.put_long( addr, data, s.data_space() )
											   : s.mem.put_word( addr, data, s.data_space() );
				
				if ( !ok )
				{
					return Bus_error;
				}
				
				addr += increment;
			}
		}
		
		if ( update_register )
		{
			s.regs[ update_register ] = addr - increment;
		}
		
		return Ok;
	}
	
	op_result microcode_MOVEM_from( processor_state& s, op_params& pb )
	{
		uint16_t mask = pb.first;
		uint32_t addr = pb.address;
		
		const uint32_t update_register = pb.target;
		
		const bool longword_sized = pb.size == long_sized;
		
		const int32_t increment = 2 << longword_sized;
		
		for ( int r = 0;  mask != 0;  ++r, mask >>= 1 )
		{
			if ( mask & 0x1 )
			{
				uint32_t data;
				
				bool ok;
				
				if ( longword_sized )
				{
					ok = s.mem.get_long( addr, data, s.data_space() );
				}
				else
				{
					uint16_t word;
					
					ok = s.mem.get_word( addr, word, s.data_space() );
					
					data = int32_t( int16_t( word ) );
				}
				
				if ( !ok )
				{
					return Bus_error;
				}
				
				s.regs[ r ] = data;
				
				addr += increment;
			}
		}
		
		if ( update_register )
		{
			s.regs[ update_register ] = addr;
		}
		
		return Ok;
	}
	
	op_result microcode_TRAP( processor_state& s, op_params& pb )
	{
		const uint32_t trap_number = pb.first;
		
		return op_result( -(trap_number + 32) );
	}
	
	op_result microcode_LINK( processor_state& s, op_params& pb )
	{
		const uint32_t n    = pb.target;
		const int32_t  disp = pb.first;
		
		uint32_t& An = s.a(n);
		uint32_t& sp = s.a(7);
		
		if ( s.badly_aligned_data( sp ) )
		{
			return Address_error;
		}
		
		sp -= 4;
		
		if ( !s.mem.put_long( sp, An, s.data_space() ) )
		{
			return Bus_error;
		}
		
		An = sp;
		
		sp += disp;
		
		return Ok;
	}
	
	op_result microcode_UNLK( processor_state& s, op_params& pb )
	{
		const uint32_t n = pb.target;
		
		uint32_t& An = s.a(n);
		uint32_t& sp = s.a(7);
		
		sp = An;
		
		if ( s.badly_aligned_data( sp ) )
		{
			return Address_error;
		}
		
		if ( !s.mem.get_long( sp, An, s.data_space() ) )
		{
			return Bus_error;
		}
		
		sp += 4;
		
		return Ok;
	}
	
	op_result microcode_MOVE_to_USP( processor_state& s, op_params& pb )
	{
		const uint32_t n = pb.target;
		
		// MOVE USP is privileged, so USP is never A7 here
		s.regs[ USP ] = s.a(n);
		
		return Ok;
	}
	
	op_result microcode_MOVE_from_USP( processor_state& s, op_params& pb )
	{
		const uint32_t n = pb.target;
		
		// MOVE USP is privileged, so USP is never A7 here
		s.a(n) = s.regs[ USP ];
		
		return Ok;
	}
	
	op_result microcode_NOP( processor_state& s, op_params& pb )
	{
		// "no operation"
		
		return Ok;
	}
	
	op_result microcode_STOP( processor_state& s, op_params& pb )
	{
		const uint32_t data = pb.first;
		
		if ( data == 0xFFFF )
		{
			s.condition = finished;
		}
		else
		{
			s.set_SR( data );
			
			s.condition = stopped;
		}
		
		return Ok;
	}
	
	op_result microcode_RTE( processor_state& s, op_params& pb )
	{
		uint32_t& sp = s.a(7);
		
		if ( s.badly_aligned_data( sp ) )
		{
			return Address_error;
		}
		
		uint16_t id;
		
		if ( !s.mem.get_word( sp + 6, id, supervisor_data_space ) )
		{
			return Bus_error;
		}
		
		const uint16_t format = id >> 12;
		
		const uint32_t stack_frame_size = format == 0 ?  8
		                                : format == 2 ? 12
		                                :                0;
		
		if ( stack_frame_size == 0 )
		{
			return Format_error;
		}
		
		if ( !s.mem.get_long( sp + 2, s.pc(), supervisor_data_space ) )
		{
			return Bus_error;
		}
		
		uint16_t saved_sr;
		
		if ( !s.mem.get_word( sp, saved_sr, supervisor_data_space ) )
		{
			return Bus_error;
		}
		
		sp += stack_frame_size;
		
		s.set_SR( saved_sr );
		
		return Ok;
	}
	
	op_result microcode_RTD( processor_state& s, op_params& pb )
	{
		microcode_RTS( s, pb );
		
		s.a(7) += pb.first;
		
		return Ok;
	}
	
	op_result microcode_RTS( processor_state& s, op_params& pb )
	{
		uint32_t& sp = s.a(7);
		
		if ( s.badly_aligned_data( sp ) )
		{
			return Address_error;
		}
		
		if ( !s.mem.get_long( sp, s.pc(), s.data_space() ) )
		{
			return Bus_error;
		}
		
		sp += 4;
		
		return Ok;
	}
	
	op_result microcode_TRAPV( processor_state& s, op_params& pb )
	{
		if ( s.get_CCR() & 0x2 )
		{
			return Overflow_trap;
		}
		
		return Ok;
	}
	
	op_result microcode_RTR( processor_state& s, op_params& pb )
	{
		uint32_t& sp = s.a(7);
		
		if ( s.badly_aligned_data( sp ) )
		{
			return Address_error;
		}
		
		uint16_t ccr;
		
		if ( !s.mem.get_word( sp, ccr, s.data_space() ) )
		{
			return Bus_error;
		}
		
		s.set_CCR( ccr );
		
		sp += 2;
		
		if ( !s.mem.get_long( sp, s.pc(), s.data_space() ) )
		{
			return Bus_error;
		}
		
		sp += 4;
		
		return Ok;
	}
	
	enum
	{
		only_3_bits = 0x1
	};
	
	static inline bool model_too_early( processor_model model, uint8_t flags )
	{
		return model < flags;
	}
	
	static inline bool model_too_late( processor_model model, uint16_t flags )
	{
		const uint8_t dropped = ~(flags >> 8);  // either FF or 40
		
		return model >= dropped;
	}
	
	static const uint16_t control_register_flags[] =
	{
		0x01,  // SFC, 3 bits
		0x01,  // DFC, 3 bits
		0x20,  // CACR
		0x40,  // TC
		0x40,  // ITT0
		0x40,  // ITT1
		0x40,  // DTT0
		0x40,  // DTT1
		
		0x00,  // USP
		0x00,  // VBR
		(~0x40 << 8) |
		0x20,  // CAAR (not 68040)
		0x20,  // MSP
		0x20,  // ISP
		0x40,  // MMUSR
		0x40,  // URP
		0x40,  // SRP
	};
	
	static inline uint16_t index_of_control_register( uint16_t id )
	{
		return id >> 8 | (id & ~0x0800);
	}
	
	static inline uint32_t* get_control_register( processor_state& s, uint16_t control_index )
	{
		return &s.regs[ control_register_offset + control_index ];
	}
	
	op_result microcode_MOVEC( processor_state& s, op_params& pb )
	{
		const uint32_t registers = pb.first;
		const uint32_t writing   = pb.second;
		
		const uint16_t general_id = registers >> 12;
		const uint16_t control_id = registers & 0x0FFF;
		
		uint32_t* control_register = 0;  // NULL
		
		const uint16_t control_index = index_of_control_register( control_id );
		
		if ( control_index & ~0x000F )
		{
			// control index is out of range; ergo, control id is invalid
		}
		else if ( uint32_t* control_register = get_control_register( s, control_index ) )
		{
			const uint16_t flags = control_register_flags[ control_index ];
			
			if ( model_too_early( s.model, flags )  ||  model_too_late( s.model, flags ) )
			{
				// This control register doesn't exist on this processor model
			}
			else
			{
				s.save_sp();  // in case we're accessing the current SP
				
				if ( writing )
				{
					uint32_t data = s.regs[ general_id ];
					
					if ( flags & only_3_bits )
					{
						data &= 0x7;
					}
					
					*control_register = data;
					
					s.load_sp();  // in case we're writing the current SP
				}
				else
				{
					s.regs[ general_id ] = *control_register;
				}
				
				return Ok;  // Success
			}
		}
		
		// Failure
		
		s.pc() -= 4;
		
		return Illegal_instruction;
	}
	
	#pragma mark -
	#pragma mark Line 5
	
	op_result microcode_DBcc( processor_state& s, op_params& pb )
	{
		const uint16_t cc = pb.second;
		
		if ( test_conditional( cc, s.sr.nzvc ) )
		{
			return Ok;
		}
		
		const uint32_t n = pb.target;
		
		const int16_t counter = s.d( n );
		
		if ( counter == 0 )
		{
			// decrement low word only, which is zero
			
			s.d( n ) += 0x0000FFFF;
		}
		else
		{
			s.d( n ) -= 1;  // decrement of non-zero word won't borrow
			
			s.pc() = pb.address;
		}
		
		return Ok;
	}
	
	op_result microcode_Scc( processor_state& s, op_params& pb )
	{
		const uint16_t cc = pb.second;
		
		pb.result = int32_t() - test_conditional( cc, s.sr.nzvc );
		
		return Ok;
	}
	
	#pragma mark -
	#pragma mark Line 6
	
	op_result microcode_BRA( processor_state& s, op_params& pb )
	{
		s.pc() = pb.address;
		
		return Ok;
	}
	
	op_result microcode_BSR( processor_state& s, op_params& pb )
	{
		uint32_t& sp = s.a(7);
		
		if ( s.badly_aligned_data( sp ) )
		{
			return Address_error;
		}
		
		sp -= 4;
		
		if ( !s.mem.put_long( sp, s.pc(), s.data_space() ) )
		{
			return Bus_error;
		}
		
		s.pc() = pb.address;
		
		return Ok;
	}
	
	op_result microcode_Bcc( processor_state& s, op_params& pb )
	{
		const uint16_t cc = pb.second;
		
		if ( test_conditional( cc, s.sr.nzvc ) )
		{
			s.pc() = pb.address;
		}
		
		return Ok;
	}
	
	#pragma mark -
	#pragma mark Line 8
	
	op_result microcode_OR( processor_state& s, op_params& pb )
	{
		pb.result = pb.first | pb.second;
		
		return Ok;
	}
	
	static uint32_t BCD_encoded( int x )
	{
		return + x / 10 << 4
		       | x % 10;
	}
	
	static int decoded_BCD( uint8_t x )
	{
		return 10 * (x >> 4) + (x & 0xF);
	}
	
	op_result microcode_SBCD( processor_state& s, op_params& pb )
	{
		const int a = decoded_BCD( pb.first  );
		const int b = decoded_BCD( pb.second );
		
		pb.result = BCD_encoded( (100 + b - a) % 100 );
		
		return Ok;
	}
	
	op_result microcode_DIVS( processor_state& s, op_params& pb )
	{
		const int32_t dividend = pb.second;
		const int16_t divisor  = pb.first;
		
		if ( divisor == 0 )
		{
			pb.size = unsized;  // Don't store the result
			
			return Division_by_zero;
		}
		
		const int32_t quotient_32 = dividend / divisor;
		
		const int16_t quotient = quotient_32;
		
		if ( quotient != quotient_32 )
		{
			pb.size = unsized;  // Don't store the result
		}
		else
		{
			//const int16_t remainder = dividend % divisor;
			const int16_t remainder = dividend - quotient * divisor;
			
			pb.result = remainder << 16 | uint16_t( quotient );
		}
		
		return Ok;
	}
	
	op_result microcode_DIVU( processor_state& s, op_params& pb )
	{
		const uint32_t dividend = pb.second;
		const uint16_t divisor  = pb.first;
		
		if ( divisor == 0 )
		{
			pb.size = unsized;  // Don't store the result
			
			return Division_by_zero;
		}
		
		const uint32_t quotient_32 = dividend / divisor;
		
		const uint16_t quotient = quotient_32;
		
		if ( quotient != quotient_32 )
		{
			pb.size = unsized;  // Don't store the result
		}
		else
		{
			//const uint16_t remainder = dividend % divisor;
			const uint16_t remainder = dividend - quotient * divisor;
			
			pb.result = remainder << 16 | quotient;
		}
		
		return Ok;
	}
	
	#pragma mark -
	#pragma mark Line 9
	
	op_result microcode_SUB( processor_state& s, op_params& pb )
	{
		const int32_t a = pb.first;
		const int32_t b = pb.second;
		
		const int32_t diff = b - a;
		
		pb.result = diff;
		
		return Ok;
	}
	
	#pragma mark -
	#pragma mark Line B
	
	op_result microcode_EOR( processor_state& s, op_params& pb )
	{
		pb.result = pb.first ^ pb.second;
		
		return Ok;
	}
	
	#pragma mark -
	#pragma mark Line C
	
	op_result microcode_AND( processor_state& s, op_params& pb )
	{
		pb.result = pb.first & pb.second;
		
		return Ok;
	}
	
	op_result microcode_EXG( processor_state& s, op_params& pb )
	{
		// Actually, these are swapped.  For "EXG A4,D0", x == 0 and y == 12.
		const uint32_t x = pb.first;
		const uint32_t y = pb.second;
		
		uint32_t& Rx = s.regs[ x ];
		uint32_t& Ry = s.regs[ y ];
		
		uint32_t temp = Rx;
		
		Rx = Ry;
		
		Ry = temp;
		
		return Ok;
	}
	
	op_result microcode_ABCD( processor_state& s, op_params& pb )
	{
		const int a = decoded_BCD( pb.first  );
		const int b = decoded_BCD( pb.second );
		
		const int sum = a + b;
		
		pb.result = BCD_encoded( sum % 100 );
		
		if ( const bool carry = sum >= 100 )
		{
			// Sick hack so ADDX-style CCR update sets the C bit
			pb.first  |= 0x80;
			pb.second |= 0x80;
		}
		
		return Ok;
	}
	
	op_result microcode_MULS( processor_state& s, op_params& pb )
	{
		pb.result = int16_t( pb.first ) * int16_t( pb.second );
		
		pb.size = long_sized;
		
		return Ok;
	}
	
	op_result microcode_MULU( processor_state& s, op_params& pb )
	{
		pb.result = uint16_t( pb.first ) * uint16_t( pb.second );
		
		pb.size = long_sized;
		
		return Ok;
	}
	
	#pragma mark -
	#pragma mark Line D
	
	op_result microcode_ADD( processor_state& s, op_params& pb )
	{
		const int32_t a = pb.first;
		const int32_t b = pb.second;
		
		const int32_t sum = a + b;
		
		pb.result = sum;
		
		return Ok;
	}
	
	op_result microcode_ASR( processor_state& s, op_params& pb )
	{
		int32_t data = pb.second;
		
		const uint16_t count = pb.first;
		
		bool last_bit = 0;
		
		if ( count != 0 )
		{
			if ( count > 32 )
			{
				data = 0;
			}
			else
			{
				data >>= count - 1;
				
				last_bit = data & 0x1;
				
				data >>= 1;
			}
			
			s.sr.x = last_bit;
		}
		
		s.sr.nzvc = N( data <  0 )
		          | Z( data == 0 )
		          | V( 0 )
		          | C( last_bit );
		
		pb.result = data;
		
		return Ok;
	}
	
	op_result microcode_ASL( processor_state& s, op_params& pb )
	{
		const uint16_t count = pb.first;
		
		int32_t data = pb.second;
		
		bool overflow = 0;
		bool last_bit = 0;
		
		if ( count != 0 )
		{
			if ( count >= 32 )
			{
				overflow = data ^ (data << 1);
				
				if ( count == 32 )
				{
					last_bit = data & 0x1;
				}
				
				data = 0;
			}
			else
			{
				// e.g. 0x0000FFFF
				const uint32_t data_mask = zero_extend( 0xFFFFFFFF, pb.size );
				
				// bshift:  0x00003FFF
				// negate:  0xFFFFC000
				// extend:  0x0000C000
				const uint32_t overflow_mask = zero_extend( ~( data_mask >> count ), pb.size );
				
				overflow = (data ^ (data << 1)) & overflow_mask;
				
				data <<= count - 1;
				
				last_bit = sign_extend( data, pb.size ) < 0;
				
				data <<= 1;
				
				data = sign_extend( data, pb.size );
			}
			
			s.sr.x = last_bit;
		}
		
		s.sr.nzvc = N( data <  0 )
		          | Z( data == 0 )
		          | V( overflow )
		          | C( last_bit );
		
		pb.result = data;
		
		return Ok;
	}
	
	op_result microcode_LSR( processor_state& s, op_params& pb )
	{
		const uint16_t count = pb.first;
		
		int32_t data = pb.second;
		
		bool last_bit = 0;
		
		if ( count != 0 )
		{
			if ( count <= 32 )
			{
				uint32_t udata = zero_extend( data, pb.size );
				
				udata >>= count - 1;
				
				last_bit = data & 0x1;
				
				udata >>= 1;
				
				data = udata;
			}
			else
			{
				data = 0;
			}
			
			s.sr.x = last_bit;
		}
		
		s.sr.nzvc = N( data <  0 )
		          | Z( data == 0 )
		          | V( 0 )
		          | C( last_bit );
		
		pb.result = data;
		
		return Ok;
	}
	
	op_result microcode_LSL( processor_state& s, op_params& pb )
	{
		const uint16_t count = pb.first;
		
		int32_t data = pb.second;
		
		bool last_bit = 0;
		
		if ( count != 0 )
		{
			if ( count <= 32 )
			{
				data = zero_extend( data, pb.size );
				
				data <<= count - 1;
				
				last_bit = sign_extend( data, pb.size ) < 0;
				
				data <<= 1;
				
				data = sign_extend( data, pb.size );
			}
			else
			{
				data = 0;
			}
			
			s.sr.x = last_bit;
		}
		
		s.sr.nzvc = N( data <  0 )
		          | Z( data == 0 )
		          | V( 0 )
		          | C( last_bit );
		
		pb.result = data;
		
		return Ok;
	}
	
	op_result microcode_ROXR( processor_state& s, op_params& pb )
	{
		const uint32_t count = pb.first;
		
		int32_t data = pb.second;
		
		if ( count != 0 )
		{
			const int n_bits = bit_count( pb.size );
			
			const int effective_count = count % (n_bits + 1);
			
			if ( effective_count != 0 )
			{
				const int anticount = n_bits - effective_count;
				
				const uint32_t udata = zero_extend( data, pb.size );
				
				data = (udata << 1 | s.sr.x) << anticount
				     |  udata                >> effective_count;
				
				s.sr.x = udata >> (effective_count - 1 ) & 0x1;
				
				data = sign_extend( data, pb.size );
			}
		}
		
		s.sr.nzvc = N( data <  0 )
		          | Z( data == 0 )
		          | V( 0 )
		          | C( s.sr.x );
		
		pb.result = data;
		
		return Ok;
	}
	
	op_result microcode_ROXL( processor_state& s, op_params& pb )
	{
		const uint32_t count = pb.first;
		
		int32_t data = pb.second;
		
		if ( count != 0 )
		{
			const int n_bits = bit_count( pb.size );
			
			const int effective_count = count % (n_bits + 1);
			
			if ( effective_count != 0 )
			{
				const int anticount = n_bits - effective_count;
				
				const uint32_t udata = zero_extend( data, pb.size );
				
				data = (udata << 1 | s.sr.x) << (effective_count - 1)
				     |  udata                >> (anticount + 1);
				
				s.sr.x = udata >> anticount & 0x1;
				
				data = sign_extend( data, pb.size );
			}
		}
		
		s.sr.nzvc = N( data <  0 )
		          | Z( data == 0 )
		          | V( 0 )
		          | C( s.sr.x );
		
		pb.result = data;
		
		return Ok;
	}
	
	op_result microcode_ROR( processor_state& s, op_params& pb )
	{
		const uint32_t count = pb.first;
		
		int32_t data = pb.second;
		
		bool last_bit = 0;
		
		if ( count != 0 )
		{
			const int n_bits = bit_count( pb.size );
			
			const int effective_count = count & n_bits - 1;
			
			if ( effective_count != 0 )
			{
				const int anticount = n_bits - effective_count;
				
				const uint32_t udata = zero_extend( data, pb.size );
				
				data = udata << anticount
				     | udata >> effective_count;
				
				data = sign_extend( data, pb.size );
			}
			
			last_bit = data < 0;
		}
		
		s.sr.nzvc = N( data <  0 )
		          | Z( data == 0 )
		          | V( 0 )
		          | C( last_bit );
		
		pb.result = data;
		
		return Ok;
	}
	
	op_result microcode_ROL( processor_state& s, op_params& pb )
	{
		const uint32_t count = pb.first;
		
		int32_t data = pb.second;
		
		bool last_bit = 0;
		
		if ( count != 0 )
		{
			const int n_bits = bit_count( pb.size );
			
			const int effective_count = count & n_bits - 1;
			
			if ( effective_count != 0 )
			{
				const int anticount = n_bits - effective_count;
				
				const uint32_t udata = zero_extend( data, pb.size );
				
				data = udata << effective_count
				     | udata >> anticount;
				
				data = sign_extend( data, pb.size );
			}
			
			last_bit = data & 0x1;
		}
		
		s.sr.nzvc = N( data <  0 )
		          | Z( data == 0 )
		          | V( 0 )
		          | C( last_bit );
		
		pb.result = data;
		
		return Ok;
	}
	
}


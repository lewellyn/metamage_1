/*
	emulator.hh
	-----------
*/

#ifndef V68K_EMULATOR_HH
#define V68K_EMULATOR_HH

// C99
#include <stdint.h>

// v68k
#include "v68k/state.hh"


namespace v68k
{
	
	class emulator : public processor_state
	{
		private:
			unsigned long its_instruction_counter;
			
			void double_bus_fault();
		
		public:
			emulator( processor_model model, const memory& mem, bkpt_handler bkpt = 0 );  // NULL
			
			unsigned long instruction_count() const  { return its_instruction_counter; }
			
			void reset();
			
			bool step();
			
			void prefetch_instruction_word();
			
			bool take_exception_format_0( uint16_t vector_offset );
		
			bool take_exception_format_2( uint16_t vector_offset, uint32_t instruction_address );
		
			bool illegal_instruction()  { return take_exception_format_0( 4 * sizeof (uint32_t) ); }
			bool privilege_violation()  { return take_exception_format_0( 8 * sizeof (uint32_t) ); }
			
			bool line_A_emulator()  { return take_exception_format_0( 10 * sizeof (uint32_t) ); }
			bool line_F_emulator()  { return take_exception_format_0( 11 * sizeof (uint32_t) ); }
	};
	
}

#endif


#ifndef FAKE6502_WRAP_H
#define FAKE6502_WRAP_H

#include <cstdint>

namespace fake6502 {


class Fake6502Wrapper {
public:
	typedef int Quit;

private:
	//6502 CPU registers
	std::uint16_t m_pc;
	std::uint8_t m_sp, m_a, m_x, m_y, m_status;


	//helper variables
	// uint32_t instructions = 0; //keep track of total instructions executed
	// uint32_t clockticks6502 = 0, clockgoal6502 = 0;
	std::uint16_t /*oldpc, */ m_ea, m_reladdr, m_value, m_result;
	std::uint8_t m_opcode /*, oldstatus */;

public:
	void reset(std::uint16_t address);
	void exec() {
		while (true) step();
	}
	void step();

protected:
	virtual std::uint8_t read6502(std::uint16_t address) = 0;
	virtual void write6502(std::uint16_t address, std::uint8_t value) = 0;
	virtual void interrupt(uint16_t) = 0;

public:
	virtual void stepHook() { }
	virtual ~Fake6502Wrapper() { }

private:
	void push16(std::uint16_t);
	void push8(std::uint8_t);
	std::uint16_t pull16();
	std::uint8_t pull8();

	std::uint16_t getvalue();
	std::uint16_t getvalue16();
	void putvalue(std::uint16_t);

	void imp(); void acc(); void imm(); void zp(); void zpx(); void zpy();
	void rel(); void abso(); void absx(); void absy(); void ind(); void indx();
	void indy();

	void adc(); void and(); void asl(); void bcc(); void bcs(); void beq();
	void bit(); void bmi(); void bne(); void bpl(); void brk(); void bvc();
	void bvs(); void clc(); void cld(); void cli(); void clv(); void cmp();
	void cpx(); void cpy(); void dec(); void dex(); void dey(); void eor();
	void inx(); void iny(); void inc(); void jmp(); void jsr(); void lda();
	void ldx(); void ldy(); void lsr(); void nop(); void ora(); void pha();
	void php(); void pla(); void plp(); void rol(); void ror(); void rti();
	void rts(); void sbc(); void sec(); void sed(); void sei(); void sta();
	void stx(); void sty(); void tax(); void tay(); void tsx(); void txa();
	void txs(); void tya();

	void dispatch();
};

}

#endif
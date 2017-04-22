/* Fake6502 CPU emulator core v1.1 *******************
 * (c)2011 Mike Chambers (miker00lz@gmail.com)       *
 *****************************************************
 * v1.1 - Small bugfix in BIT opcode, but it was the *
 *        difference between a few games in my NES   *
 *        emulator working and being broken!         *
 *        I went through the rest carefully again    *
 *        after fixing it just to make sure I didn't *
 *        have any other typos! (Dec. 17, 2011)      *
 *                                                   *
 * v1.0 - First release (Nov. 24, 2011)              *
 *****************************************************
 * LICENSE: This source code is released into the    *
 * public domain, but if you use it please do give   *
 * credit. I put a lot of effort into writing this!  *
 *                                                   *
 *****************************************************
 * Fake6502 is a MOS Technology 6502 CPU emulation   *
 * engine in C. It was written as part of a Nintendo *
 * Entertainment System emulator I've been writing.  *
 *                                                   *
 * A couple important things to know about are two   *
 * defines in the code. One is "UNDOCUMENTED" which, *
 * when defined, allows Fake6502 to compile with     *
 * full support for the more predictable             *
 * undocumented instructions of the 6502. If it is   *
 * undefined, undocumented opcodes just act as NOPs. *
 *                                                   *
 * The other define is "NES_CPU", which causes the   *
 * code to compile without support for binary-coded  *
 * decimal (BCD) support for the ADC and SBC         *
 * opcodes. The Ricoh 2A03 CPU in the NES does not   *
 * support BCD, but is otherwise identical to the    *
 * standard MOS 6502. (Note that this define is      *
 * enabled in this file if you haven't changed it    *
 * yourself. If you're not emulating a NES, you      *
 * should comment it out.)                           *
 *                                                   *
 * If you do discover an error in timing accuracy,   *
 * or operation in general please e-mail me at the   *
 * address above so that I can fix it. Thank you!    *
 *                                                   *
 *****************************************************
 * Usage:                                            *
 *                                                   *
 * Fake6502 requires you to provide two external     *
 * functions:                                        *
 *                                                   *
 * uint8_t read6502(uint16_t address)                *
 * void write6502(uint16_t address, uint8_t value)   *
 *                                                   *
 * You may optionally pass Fake6502 the pointer to a *
 * function which you want to be called after every  *
 * emulated instruction. This function should be a   *
 * void with no parameters expected to be passed to  *
 * it.                                               *
 *                                                   *
 * This can be very useful. For example, in a NES    *
 * emulator, you check the number of clock ticks     *
 * that have passed so you can know when to handle   *
 * APU events.                                       *
 *                                                   *
 * To pass Fake6502 this pointer, use the            *
 * hookexternal(void *funcptr) function provided.    *
 *                                                   *
 * To disable the hook later, pass NULL to it.       *
 *****************************************************
 * Useful functions in this emulator:                *
 *                                                   *
 * void reset6502()                                  *
 *   - Call this once before you begin execution.    *
 *                                                   *
 * void exec6502(uint32_t tickcount)                 *
 *   - Execute 6502 code up to the next specified    *
 *     count of clock ticks.                         *
 *                                                   *
 * void step6502()                                   *
 *   - Execute a single instrution.                  *
 *                                                   *
 * void irq6502()                                    *
 *   - Trigger a hardware IRQ in the 6502 core.      *
 *                                                   *
 * void nmi6502()                                    *
 *   - Trigger an NMI in the 6502 core.              *
 *                                                   *
 * void hookexternal(void *funcptr)                  *
 *   - Pass a pointer to a void function taking no   *
 *     parameters. This will cause Fake6502 to call  *
 *     that function once after each emulated        *
 *     instruction.                                  *
 *                                                   *
 *****************************************************
 * Useful variables in this emulator:                *
 *                                                   *
 * uint32_t clockticks6502                           *
 *   - A running total of the emulated cycle count.  *
 *                                                   *
 * uint32_t instructions                             *
 *   - A running total of the total emulated         *
 *     instruction count. This is not related to     *
 *     clock cycle timing.                           *
 *                                                   *
 *****************************************************/

#include "fake6502_wrap.h"

using namespace std;
using namespace fake6502;

//6502 defines
// #define UNDOCUMENTED //when this is defined, undocumented opcodes are handled.
                     //otherwise, they're simply treated as NOPs.

#define NES_CPU      //when this is defined, the binary-coded decimal (BCD)
                     //status flag is not honored by ADC and SBC. the 2A03
                     //CPU in the Nintendo Entertainment System does not
                     //support BCD operation.

#define FLAG_CARRY     0x01
#define FLAG_ZERO      0x02
#define FLAG_INTERRUPT 0x04
#define FLAG_DECIMAL   0x08
#define FLAG_BREAK     0x10
#define FLAG_CONSTANT  0x20
#define FLAG_OVERFLOW  0x40
#define FLAG_SIGN      0x80

#define BASE_STACK     0x100

#define saveaccum(n) m_a = (uint8_t)((n) & 0x00FF)


//flag modifier macros
#define setcarry() m_status |= FLAG_CARRY
#define clearcarry() m_status &= (~FLAG_CARRY)
#define setzero() m_status |= FLAG_ZERO
#define clearzero() m_status &= (~FLAG_ZERO)
#define setinterrupt() m_status |= FLAG_INTERRUPT
#define clearinterrupt() m_status &= (~FLAG_INTERRUPT)
#define setdecimal() m_status |= FLAG_DECIMAL
#define cleardecimal() m_status &= (~FLAG_DECIMAL)
#define setoverflow() m_status |= FLAG_OVERFLOW
#define clearoverflow() m_status &= (~FLAG_OVERFLOW)
#define setsign() m_status |= FLAG_SIGN
#define clearsign() m_status &= (~FLAG_SIGN)


//flag calculation macros
#define zerocalc(n) {\
    if ((n) & 0x00FF) clearzero();\
        else setzero();\
}

#define signcalc(n) {\
    if ((n) & 0x0080) setsign();\
        else clearsign();\
}

#define carrycalc(n) {\
    if ((n) & 0xFF00) setcarry();\
        else clearcarry();\
}

#define overflowcalc(n, m, o) { /* n = result, m = accumulator, o = memory */ \
    if (((n) ^ (uint16_t)(m)) & ((n) ^ (o)) & 0x0080) setoverflow();\
        else clearoverflow();\
}


//a few general functions used by various other functions
inline void Fake6502Wrapper::push16(uint16_t pushval) {
    write6502(BASE_STACK + m_sp, (pushval >> 8) & 0xFF);
    write6502(BASE_STACK + ((m_sp - 1) & 0xFF), pushval & 0xFF);
    m_sp -= 2;
}

inline void Fake6502Wrapper::push8(uint8_t pushval) {
    write6502(BASE_STACK + m_sp--, pushval);
}

inline uint16_t Fake6502Wrapper::pull16() {
    uint16_t temp16;
    temp16 = read6502(BASE_STACK + ((m_sp + 1) & 0xFF)) | ((uint16_t)read6502(BASE_STACK + ((m_sp + 2) & 0xFF)) << 8);
    m_sp += 2;
    return(temp16);
}

inline uint8_t Fake6502Wrapper::pull8() {
    return (read6502(BASE_STACK + ++m_sp));
}

void Fake6502Wrapper::reset(uint16_t address) {
	m_pc = address;
    // pc = (uint16_t)read6502(0xFFFC) | ((uint16_t)read6502(0xFFFD) << 8);
    m_a = 0;
    m_x = 0;
    m_y = 0;
    // sp = 0xFD;
	m_sp = 0xff;
	push16(0xffff);
    m_status |= FLAG_CONSTANT;
}


// namespace {
// 	extern void (*addrtable[256])();
// 	extern void (*optable[256])();
// }
// uint8_t penaltyop, penaltyaddr;

//addressing mode functions, calculates effective addresses
inline void Fake6502Wrapper::imp() { //implied
}

inline void Fake6502Wrapper::acc() { //accumulator
}

inline void Fake6502Wrapper::imm() { //immediate
    m_ea = m_pc++;
}

inline void Fake6502Wrapper::zp() { //zero-page
    m_ea = (uint16_t)read6502((uint16_t)m_pc++);
}

inline void Fake6502Wrapper::zpx() { //zero-page,X
    m_ea = ((uint16_t)read6502((uint16_t)m_pc++) + (uint16_t)m_x) & 0xFF; //zero-page wraparound
}

inline void Fake6502Wrapper::zpy() { //zero-page,Y
    m_ea = ((uint16_t)read6502((uint16_t)m_pc++) + (uint16_t)m_y) & 0xFF; //zero-page wraparound
}

inline void Fake6502Wrapper::rel() { //relative for branch ops (8-bit immediate value, sign-extended)
    m_reladdr = (uint16_t)read6502(m_pc++);
    if (m_reladdr & 0x80) m_reladdr |= 0xFF00;
}

inline void Fake6502Wrapper::abso() { //absolute
    m_ea = (uint16_t)read6502(m_pc) | ((uint16_t)read6502(m_pc+1) << 8);
    m_pc += 2;
}

inline void Fake6502Wrapper::absx() { //absolute,X
    uint16_t startpage;
    m_ea = ((uint16_t)read6502(m_pc) | ((uint16_t)read6502(m_pc+1) << 8));
    startpage = m_ea & 0xFF00;
    m_ea += (uint16_t)m_x;

    // if (startpage != (ea & 0xFF00)) { //one cycle penlty for page-crossing on some opcodes
    //     penaltyaddr = 1;
    // }

    m_pc += 2;
}

inline void Fake6502Wrapper::absy() { //absolute,Y
    uint16_t startpage;
    m_ea = ((uint16_t)read6502(m_pc) | ((uint16_t)read6502(m_pc+1) << 8));
    startpage = m_ea & 0xFF00;
    m_ea += (uint16_t)m_y;

    // if (startpage != (ea & 0xFF00)) { //one cycle penlty for page-crossing on some opcodes
    //     penaltyaddr = 1;
    // }

    m_pc += 2;
}

inline void Fake6502Wrapper::ind() { //indirect
    uint16_t eahelp, eahelp2;
    eahelp = (uint16_t)read6502(m_pc) | (uint16_t)((uint16_t)read6502(m_pc+1) << 8);
    eahelp2 = (eahelp & 0xFF00) | ((eahelp + 1) & 0x00FF); //replicate 6502 page-boundary wraparound bug
    m_ea = (uint16_t)read6502(eahelp) | ((uint16_t)read6502(eahelp2) << 8);
    m_pc += 2;
}

inline void Fake6502Wrapper::indx() { // (indirect,X)
    uint16_t eahelp;
    eahelp = (uint16_t)(((uint16_t)read6502(m_pc++) + (uint16_t)m_x) & 0xFF); //zero-page wraparound for table pointer
    m_ea = (uint16_t)read6502(eahelp & 0x00FF) | ((uint16_t)read6502((eahelp+1) & 0x00FF) << 8);
}

inline void Fake6502Wrapper::indy() { // (indirect),Y
    uint16_t eahelp, eahelp2, startpage;
    eahelp = (uint16_t)read6502(m_pc++);
    eahelp2 = (eahelp & 0xFF00) | ((eahelp + 1) & 0x00FF); //zero-page wraparound
    m_ea = (uint16_t)read6502(eahelp) | ((uint16_t)read6502(eahelp2) << 8);
    startpage = m_ea & 0xFF00;
    m_ea += (uint16_t)m_y;

    // if (startpage != (ea & 0xFF00)) { //one cycle penlty for page-crossing on some opcodes
    //     penaltyaddr = 1;
    // }
}

inline uint16_t Fake6502Wrapper::getvalue() {
    if (0xa == m_opcode || 0x2a == m_opcode || 0x4a == m_opcode || 0x6a == m_opcode)
		return((uint16_t)m_a);
	else return((uint16_t)read6502(m_ea));
}

inline uint16_t Fake6502Wrapper::getvalue16() {
    return((uint16_t)read6502(m_ea) | ((uint16_t)read6502(m_ea+1) << 8));
}

inline void Fake6502Wrapper::putvalue(uint16_t saveval) {
    if (0xa == m_opcode || 0x2a == m_opcode || 0x4a == m_opcode || 0x6a == m_opcode)
		m_a = (uint8_t)(saveval & 0x00FF);
	else write6502(m_ea, (saveval & 0x00FF));
}


//instruction handler functions
inline void Fake6502Wrapper::adc() {
    // penaltyop = 1;
    m_value = getvalue();
    m_result = (uint16_t)m_a + m_value + (uint16_t)(m_status & FLAG_CARRY);
   
    carrycalc(m_result);
    zerocalc(m_result);
    overflowcalc(m_result, m_a, m_value);
    signcalc(m_result);
    
    #ifndef NES_CPU
    if (status & FLAG_DECIMAL) {
        clearcarry();
        
        if ((a & 0x0F) > 0x09) {
            a += 0x06;
        }
        if ((a & 0xF0) > 0x90) {
            a += 0x60;
            setcarry();
        }
        
        clockticks6502++;
    }
    #endif
   
    saveaccum(m_result);
}

inline void Fake6502Wrapper::and() {
    // penaltyop = 1;
    m_value = getvalue();
    m_result = (uint16_t)m_a & m_value;
   
    zerocalc(m_result);
    signcalc(m_result);
   
    saveaccum(m_result);
}

inline void Fake6502Wrapper::asl() {
    m_value = getvalue();
    m_result = m_value << 1;

    carrycalc(m_result);
    zerocalc(m_result);
    signcalc(m_result);
   
    putvalue(m_result);
}

inline void Fake6502Wrapper::bcc() {
    if ((m_status & FLAG_CARRY) == 0) {
        // oldpc = pc;
        m_pc += m_reladdr;
        // if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
        //    else clockticks6502++;
    }
}

inline void Fake6502Wrapper::bcs() {
    if ((m_status & FLAG_CARRY) == FLAG_CARRY) {
        // oldpc = pc;
        m_pc += m_reladdr;
        // if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
        //    else clockticks6502++;
    }
}

inline void Fake6502Wrapper::beq() {
    if ((m_status & FLAG_ZERO) == FLAG_ZERO) {
        // oldpc = pc;
        m_pc += m_reladdr;
        // if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
        //     else clockticks6502++;
    }
}

inline void Fake6502Wrapper::bit() {
    m_value = getvalue();
    m_result = (uint16_t)m_a & m_value;
   
    zerocalc(m_result);
    m_status = (m_status & 0x3F) | (uint8_t)(m_value & 0xC0);
}

inline void Fake6502Wrapper::bmi() {
    if ((m_status & FLAG_SIGN) == FLAG_SIGN) {
        // oldpc = pc;
        m_pc += m_reladdr;
        // if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
        //    else clockticks6502++;
    }
}

inline void Fake6502Wrapper::bne() {
    if ((m_status & FLAG_ZERO) == 0) {
        // oldpc = pc;
        m_pc += m_reladdr;
        // if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
        //    else clockticks6502++;
    }
}

inline void Fake6502Wrapper::bpl() {
    if ((m_status & FLAG_SIGN) == 0) {
        // oldpc = pc;
        m_pc += m_reladdr;
        // if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
        //    else clockticks6502++;
    }
}

inline void Fake6502Wrapper::brk() {
    // m_pc++;
    // push16(m_pc); //push next instruction address onto stack
    // push8(m_status | FLAG_BREAK); //push CPU status to stack
    // setinterrupt(); //set interrupt flag
    // m_pc = (uint16_t)read6502(0xFFFE) | ((uint16_t)read6502(0xFFFF) << 8);
	interrupt(m_ea);
}

inline void Fake6502Wrapper::bvc() {
    if ((m_status & FLAG_OVERFLOW) == 0) {
        // oldpc = pc;
        m_pc += m_reladdr;
        // if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
        //    else clockticks6502++;
    }
}

inline void Fake6502Wrapper::bvs() {
    if ((m_status & FLAG_OVERFLOW) == FLAG_OVERFLOW) {
        // oldpc = pc;
        m_pc += m_reladdr;
        // if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
        //    else clockticks6502++;
    }
}

inline void Fake6502Wrapper::clc() {
    clearcarry();
}

inline void Fake6502Wrapper::cld() {
    cleardecimal();
}

inline void Fake6502Wrapper::cli() {
    clearinterrupt();
}

inline void Fake6502Wrapper::clv() {
    clearoverflow();
}

inline void Fake6502Wrapper::cmp() {
    // penaltyop = 1;
    m_value = getvalue();
    m_result = (uint16_t)m_a - m_value;
   
    if (m_a >= (uint8_t)(m_value & 0x00FF)) setcarry();
        else clearcarry();
    if (m_a == (uint8_t)(m_value & 0x00FF)) setzero();
        else clearzero();
    signcalc(m_result);
}

inline void Fake6502Wrapper::cpx() {
    m_value = getvalue();
    m_result = (uint16_t)m_x - m_value;
   
    if (m_x >= (uint8_t)(m_value & 0x00FF)) setcarry();
        else clearcarry();
    if (m_x == (uint8_t)(m_value & 0x00FF)) setzero();
        else clearzero();
    signcalc(m_result);
}

inline void Fake6502Wrapper::cpy() {
    m_value = getvalue();
    m_result = (uint16_t)m_y - m_value;
   
    if (m_y >= (uint8_t)(m_value & 0x00FF)) setcarry();
        else clearcarry();
    if (m_y == (uint8_t)(m_value & 0x00FF)) setzero();
        else clearzero();
    signcalc(m_result);
}

inline void Fake6502Wrapper::dec() {
    m_value = getvalue();
    m_result = m_value - 1;
   
    zerocalc(m_result);
    signcalc(m_result);
   
    putvalue(m_result);
}

inline void Fake6502Wrapper::dex() {
    m_x--;
   
    zerocalc(m_x);
    signcalc(m_x);
}

inline void Fake6502Wrapper::dey() {
    m_y--;
   
    zerocalc(m_y);
    signcalc(m_y);
}

inline void Fake6502Wrapper::eor() {
    // penaltyop = 1;
    m_value = getvalue();
    m_result = (uint16_t)m_a ^ m_value;
   
    zerocalc(m_result);
    signcalc(m_result);
   
    saveaccum(m_result);
}

inline void Fake6502Wrapper::inc() {
    m_value = getvalue();
    m_result = m_value + 1;
   
    zerocalc(m_result);
    signcalc(m_result);
   
    putvalue(m_result);
}

inline void Fake6502Wrapper::inx() {
    m_x++;
   
    zerocalc(m_x);
    signcalc(m_x);
}

inline void Fake6502Wrapper::iny() {
    m_y++;
   
    zerocalc(m_y);
    signcalc(m_y);
}

inline void Fake6502Wrapper::jmp() {
    m_pc = m_ea;
}

inline void Fake6502Wrapper::jsr() {
    push16(m_pc - 1);
    m_pc = m_ea;
}

inline void Fake6502Wrapper::lda() {
    // penaltyop = 1;
    m_value = getvalue();
    m_a = (uint8_t)(m_value & 0x00FF);
   
    zerocalc(m_a);
    signcalc(m_a);
}

inline void Fake6502Wrapper::ldx() {
    // penaltyop = 1;
    m_value = getvalue();
    m_x = (uint8_t)(m_value & 0x00FF);
   
    zerocalc(m_x);
    signcalc(m_x);
}

inline void Fake6502Wrapper::ldy() {
    // penaltyop = 1;
    m_value = getvalue();
    m_y = (uint8_t)(m_value & 0x00FF);
   
    zerocalc(m_y);
    signcalc(m_y);
}

inline void Fake6502Wrapper::lsr() {
    m_value = getvalue();
    m_result = m_value >> 1;
   
    if (m_value & 1) setcarry();
        else clearcarry();
    zerocalc(m_result);
    signcalc(m_result);
   
    putvalue(m_result);
}

inline void Fake6502Wrapper::nop() {
	/*
    switch (opcode) {
        case 0x1C:
        case 0x3C:
        case 0x5C:
        case 0x7C:
        case 0xDC:
        case 0xFC:
            penaltyop = 1;
            break;
    }
	*/
}

inline void Fake6502Wrapper::ora() {
    // penaltyop = 1;
    m_value = getvalue();
    m_result = (uint16_t)m_a | m_value;
   
    zerocalc(m_result);
    signcalc(m_result);
   
    saveaccum(m_result);
}

inline void Fake6502Wrapper::pha() {
    push8(m_a);
}

inline void Fake6502Wrapper::php() {
    push8(m_status | FLAG_BREAK);
}

inline void Fake6502Wrapper::pla() {
    m_a = pull8();
   
    zerocalc(m_a);
    signcalc(m_a);
}

inline void Fake6502Wrapper::plp() {
    m_status = pull8() | FLAG_CONSTANT;
}

inline void Fake6502Wrapper::rol() {
    m_value = getvalue();
    m_result = (m_value << 1) | (m_status & FLAG_CARRY);
   
    carrycalc(m_result);
    zerocalc(m_result);
    signcalc(m_result);
   
    putvalue(m_result);
}

inline void Fake6502Wrapper::ror() {
    m_value = getvalue();
    m_result = (m_value >> 1) | ((m_status & FLAG_CARRY) << 7);
   
    if (m_value & 1) setcarry();
        else clearcarry();
    zerocalc(m_result);
    signcalc(m_result);
   
    putvalue(m_result);
}

inline void Fake6502Wrapper::rti() {
    m_status = pull8();
    m_value = pull16();
    m_pc = m_value;
}

inline void Fake6502Wrapper::rts() {
    m_value = pull16();
    m_pc = m_value + 1;
	if (0 == m_pc)
		throw Quit();
}

inline void Fake6502Wrapper::sbc() {
    // penaltyop = 1;
    m_value = getvalue() ^ 0x00FF;
    m_result = (uint16_t)m_a + m_value + (uint16_t)(m_status & FLAG_CARRY);
   
    carrycalc(m_result);
    zerocalc(m_result);
    overflowcalc(m_result, m_a, m_value);
    signcalc(m_result);

    #ifndef NES_CPU
    if (status & FLAG_DECIMAL) {
        clearcarry();
        
        a -= 0x66;
        if ((a & 0x0F) > 0x09) {
            a += 0x06;
        }
        if ((a & 0xF0) > 0x90) {
            a += 0x60;
            setcarry();
        }
        
        clockticks6502++;
    }
    #endif
   
    saveaccum(m_result);
}

inline void Fake6502Wrapper::sec() {
    setcarry();
}

inline void Fake6502Wrapper::sed() {
    setdecimal();
}

inline void Fake6502Wrapper::sei() {
    setinterrupt();
}

inline void Fake6502Wrapper::sta() {
    putvalue(m_a);
}

inline void Fake6502Wrapper::stx() {
    putvalue(m_x);
}

inline void Fake6502Wrapper::sty() {
    putvalue(m_y);
}

inline void Fake6502Wrapper::tax() {
    m_x = m_a;
   
    zerocalc(m_x);
    signcalc(m_x);
}

inline void Fake6502Wrapper::tay() {
    m_y = m_a;
   
    zerocalc(m_y);
    signcalc(m_y);
}

inline void Fake6502Wrapper::tsx() {
    m_x = m_sp;
   
    zerocalc(m_x);
    signcalc(m_x);
}

inline void Fake6502Wrapper::txa() {
    m_a = m_x;
   
    zerocalc(m_a);
    signcalc(m_a);
}

inline void Fake6502Wrapper::txs() {
    m_sp = m_x;
}

inline void Fake6502Wrapper::tya() {
    m_a = m_y;
   
    zerocalc(m_a);
    signcalc(m_a);
}

//undocumented instructions
#ifdef UNDOCUMENTED
    static void lax() {
        lda();
        ldx();
    }

    static void sax() {
        sta();
        stx();
        putvalue(a & x);
        if (penaltyop && penaltyaddr) clockticks6502--;
    }

    static void dcp() {
        dec();
        cmp();
        if (penaltyop && penaltyaddr) clockticks6502--;
    }

    static void isb() {
        inc();
        sbc();
        if (penaltyop && penaltyaddr) clockticks6502--;
    }

    static void slo() {
        asl();
        ora();
        if (penaltyop && penaltyaddr) clockticks6502--;
    }

    static void rla() {
        rol();
        and();
        if (penaltyop && penaltyaddr) clockticks6502--;
    }

    static void sre() {
        lsr();
        eor();
        if (penaltyop && penaltyaddr) clockticks6502--;
    }

    static void rra() {
        ror();
        adc();
        if (penaltyop && penaltyaddr) clockticks6502--;
    }
#else
    #define lax nop
    #define sax nop
    #define dcp nop
    #define isb nop
    #define slo nop
    #define rla nop
    #define sre nop
    #define rra nop
#endif

// namespace {
// void (*addrtable[256])() = {
// /*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
// /* 0 */     imp, indx,  imp, indx,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imm, abso, abso, abso, abso, /* 0 */
// /* 1 */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* 1 */
// /* 2 */    abso, indx,  imp, indx,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imm, abso, abso, abso, abso, /* 2 */
// /* 3 */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* 3 */
// /* 4 */     imp, indx,  imp, indx,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imm, abso, abso, abso, abso, /* 4 */
// /* 5 */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* 5 */
// /* 6 */     imp, indx,  imp, indx,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imm,  ind, abso, abso, abso, /* 6 */
// /* 7 */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* 7 */
// /* 8 */     imm, indx,  imm, indx,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imm, abso, abso, abso, abso, /* 8 */
// /* 9 */     rel, indy,  imp, indy,  zpx,  zpx,  zpy,  zpy,  imp, absy,  imp, absy, absx, absx, absy, absy, /* 9 */
// /* A */     imm, indx,  imm, indx,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imm, abso, abso, abso, abso, /* A */
// /* B */     rel, indy,  imp, indy,  zpx,  zpx,  zpy,  zpy,  imp, absy,  imp, absy, absx, absx, absy, absy, /* B */
// /* C */     imm, indx,  imm, indx,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imm, abso, abso, abso, abso, /* C */
// /* D */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* D */
// /* E */     imm, indx,  imm, indx,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imm, abso, abso, abso, abso, /* E */
// /* F */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx  /* F */
// };

// void (*optable[256])() = {
// /*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |      */
// /* 0 */      brk,  ora,  nop,  slo,  nop,  ora,  asl,  slo,  php,  ora,  asl,  nop,  nop,  ora,  asl,  slo, /* 0 */
// /* 1 */      bpl,  ora,  nop,  slo,  nop,  ora,  asl,  slo,  clc,  ora,  nop,  slo,  nop,  ora,  asl,  slo, /* 1 */
// /* 2 */      jsr,  and,  nop,  rla,  bit,  and,  rol,  rla,  plp,  and,  rol,  nop,  bit,  and,  rol,  rla, /* 2 */
// /* 3 */      bmi,  and,  nop,  rla,  nop,  and,  rol,  rla,  sec,  and,  nop,  rla,  nop,  and,  rol,  rla, /* 3 */
// /* 4 */      rti,  eor,  nop,  sre,  nop,  eor,  lsr,  sre,  pha,  eor,  lsr,  nop,  jmp,  eor,  lsr,  sre, /* 4 */
// /* 5 */      bvc,  eor,  nop,  sre,  nop,  eor,  lsr,  sre,  cli,  eor,  nop,  sre,  nop,  eor,  lsr,  sre, /* 5 */
// /* 6 */      rts,  adc,  nop,  rra,  nop,  adc,  ror,  rra,  pla,  adc,  ror,  nop,  jmp,  adc,  ror,  rra, /* 6 */
// /* 7 */      bvs,  adc,  nop,  rra,  nop,  adc,  ror,  rra,  sei,  adc,  nop,  rra,  nop,  adc,  ror,  rra, /* 7 */
// /* 8 */      nop,  sta,  nop,  sax,  sty,  sta,  stx,  sax,  dey,  nop,  txa,  nop,  sty,  sta,  stx,  sax, /* 8 */
// /* 9 */      bcc,  sta,  nop,  nop,  sty,  sta,  stx,  sax,  tya,  sta,  txs,  nop,  nop,  sta,  nop,  nop, /* 9 */
// /* A */      ldy,  lda,  ldx,  lax,  ldy,  lda,  ldx,  lax,  tay,  lda,  tax,  nop,  ldy,  lda,  ldx,  lax, /* A */
// /* B */      bcs,  lda,  nop,  lax,  ldy,  lda,  ldx,  lax,  clv,  lda,  tsx,  lax,  ldy,  lda,  ldx,  lax, /* B */
// /* C */      cpy,  cmp,  nop,  dcp,  cpy,  cmp,  dec,  dcp,  iny,  cmp,  dex,  nop,  cpy,  cmp,  dec,  dcp, /* C */
// /* D */      bne,  cmp,  nop,  dcp,  nop,  cmp,  dec,  dcp,  cld,  cmp,  nop,  dcp,  nop,  cmp,  dec,  dcp, /* D */
// /* E */      cpx,  sbc,  nop,  isb,  cpx,  sbc,  inc,  isb,  inx,  sbc,  nop,  sbc,  cpx,  sbc,  inc,  isb, /* E */
// /* F */      beq,  sbc,  nop,  isb,  nop,  sbc,  inc,  isb,  sed,  sbc,  nop,  isb,  nop,  sbc,  inc,  isb  /* F */
// };

// }


//static const uint32_t ticktable[256] = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
// /* 0 */      7,    6,    2,    8,    3,    3,    5,    5,    3,    2,    2,    2,    4,    4,    6,    6,  /* 0 */
// /* 1 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 1 */
// /* 2 */      6,    6,    2,    8,    3,    3,    5,    5,    4,    2,    2,    2,    4,    4,    6,    6,  /* 2 */
// /* 3 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 3 */
// /* 4 */      6,    6,    2,    8,    3,    3,    5,    5,    3,    2,    2,    2,    3,    4,    6,    6,  /* 4 */
// /* 5 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 5 */
// /* 6 */      6,    6,    2,    8,    3,    3,    5,    5,    4,    2,    2,    2,    5,    4,    6,    6,  /* 6 */
// /* 7 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 7 */
// /* 8 */      2,    6,    2,    6,    3,    3,    3,    3,    2,    2,    2,    2,    4,    4,    4,    4,  /* 8 */
// /* 9 */      2,    6,    2,    6,    4,    4,    4,    4,    2,    5,    2,    5,    5,    5,    5,    5,  /* 9 */
// /* A */      2,    6,    2,    6,    3,    3,    3,    3,    2,    2,    2,    2,    4,    4,    4,    4,  /* A */
// /* B */      2,    5,    2,    5,    4,    4,    4,    4,    2,    4,    2,    4,    4,    4,    4,    4,  /* B */
// /* C */      2,    6,    2,    8,    3,    3,    5,    5,    2,    2,    2,    2,    4,    4,    6,    6,  /* C */
// /* D */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* D */
// /* E */      2,    6,    2,    8,    3,    3,    5,    5,    2,    2,    2,    2,    4,    4,    6,    6,  /* E */
// /* F */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7   /* F */
// };


// void nmi6502() {
//     push16(pc);
//     push8(status);
//     status |= FLAG_INTERRUPT;
//     pc = (uint16_t)read6502(0xFFFA) | ((uint16_t)read6502(0xFFFB) << 8);
// }

// void irq6502() {
//     push16(pc);
//     push8(status);
//     status |= FLAG_INTERRUPT;
//     pc = (uint16_t)read6502(0xFFFE) | ((uint16_t)read6502(0xFFFF) << 8);
// }

// uint8_t callexternal = 0;
// void (*loopexternal)();

inline void Fake6502Wrapper::dispatch() {
	switch (m_opcode) {
	// case 0x00: imp(); brk(); break;
	case 0x00: abso(); brk(); break;
	case 0x01: indx(); ora(); break;
	case 0x02: imp(); nop(); break;
	case 0x03: indx(); slo(); break;
	case 0x04: zp(); nop(); break;
	case 0x05: zp(); ora(); break;
	case 0x06: zp(); asl(); break;
	case 0x07: zp(); slo(); break;
	case 0x08: imp(); php(); break;
	case 0x09: imm(); ora(); break;
	case 0x0A: acc(); asl(); break;
	case 0x0B: imm(); nop(); break;
	case 0x0C: abso(); nop(); break;
	case 0x0D: abso(); ora(); break;
	case 0x0E: abso(); asl(); break;
	case 0x0F: abso(); slo(); break;
	case 0x10: rel(); bpl(); break;
	case 0x11: indy(); ora(); break;
	case 0x12: imp(); nop(); break;
	case 0x13: indy(); slo(); break;
	case 0x14: zpx(); nop(); break;
	case 0x15: zpx(); ora(); break;
	case 0x16: zpx(); asl(); break;
	case 0x17: zpx(); slo(); break;
	case 0x18: imp(); clc(); break;
	case 0x19: absy(); ora(); break;
	case 0x1A: imp(); nop(); break;
	case 0x1B: absy(); slo(); break;
	case 0x1C: absx(); nop(); break;
	case 0x1D: absx(); ora(); break;
	case 0x1E: absx(); asl(); break;
	case 0x1F: absx(); slo(); break;
	case 0x20: abso(); jsr(); break;
	case 0x21: indx(); and (); break;
	case 0x22: imp(); nop(); break;
	case 0x23: indx(); rla(); break;
	case 0x24: zp(); bit(); break;
	case 0x25: zp(); and (); break;
	case 0x26: zp(); rol(); break;
	case 0x27: zp(); rla(); break;
	case 0x28: imp(); plp(); break;
	case 0x29: imm(); and (); break;
	case 0x2A: acc(); rol(); break;
	case 0x2B: imm(); nop(); break;
	case 0x2C: abso(); bit(); break;
	case 0x2D: abso(); and (); break;
	case 0x2E: abso(); rol(); break;
	case 0x2F: abso(); rla(); break;
	case 0x30: rel(); bmi(); break;
	case 0x31: indy(); and (); break;
	case 0x32: imp(); nop(); break;
	case 0x33: indy(); rla(); break;
	case 0x34: zpx(); nop(); break;
	case 0x35: zpx(); and (); break;
	case 0x36: zpx(); rol(); break;
	case 0x37: zpx(); rla(); break;
	case 0x38: imp(); sec(); break;
	case 0x39: absy(); and (); break;
	case 0x3A: imp(); nop(); break;
	case 0x3B: absy(); rla(); break;
	case 0x3C: absx(); nop(); break;
	case 0x3D: absx(); and (); break;
	case 0x3E: absx(); rol(); break;
	case 0x3F: absx(); rla(); break;
	case 0x40: imp(); rti(); break;
	case 0x41: indx(); eor(); break;
	case 0x42: imp(); nop(); break;
	case 0x43: indx(); sre(); break;
	case 0x44: zp(); nop(); break;
	case 0x45: zp(); eor(); break;
	case 0x46: zp(); lsr(); break;
	case 0x47: zp(); sre(); break;
	case 0x48: imp(); pha(); break;
	case 0x49: imm(); eor(); break;
	case 0x4A: acc(); lsr(); break;
	case 0x4B: imm(); nop(); break;
	case 0x4C: abso(); jmp(); break;
	case 0x4D: abso(); eor(); break;
	case 0x4E: abso(); lsr(); break;
	case 0x4F: abso(); sre(); break;
	case 0x50: rel(); bvc(); break;
	case 0x51: indy(); eor(); break;
	case 0x52: imp(); nop(); break;
	case 0x53: indy(); sre(); break;
	case 0x54: zpx(); nop(); break;
	case 0x55: zpx(); eor(); break;
	case 0x56: zpx(); lsr(); break;
	case 0x57: zpx(); sre(); break;
	case 0x58: imp(); cli(); break;
	case 0x59: absy(); eor(); break;
	case 0x5A: imp(); nop(); break;
	case 0x5B: absy(); sre(); break;
	case 0x5C: absx(); nop(); break;
	case 0x5D: absx(); eor(); break;
	case 0x5E: absx(); lsr(); break;
	case 0x5F: absx(); sre(); break;
	case 0x60: imp(); rts(); break;
	case 0x61: indx(); adc(); break;
	case 0x62: imp(); nop(); break;
	case 0x63: indx(); rra(); break;
	case 0x64: zp(); nop(); break;
	case 0x65: zp(); adc(); break;
	case 0x66: zp(); ror(); break;
	case 0x67: zp(); rra(); break;
	case 0x68: imp(); pla(); break;
	case 0x69: imm(); adc(); break;
	case 0x6A: acc(); ror(); break;
	case 0x6B: imm(); nop(); break;
	case 0x6C: ind(); jmp(); break;
	case 0x6D: abso(); adc(); break;
	case 0x6E: abso(); ror(); break;
	case 0x6F: abso(); rra(); break;
	case 0x70: rel(); bvs(); break;
	case 0x71: indy(); adc(); break;
	case 0x72: imp(); nop(); break;
	case 0x73: indy(); rra(); break;
	case 0x74: zpx(); nop(); break;
	case 0x75: zpx(); adc(); break;
	case 0x76: zpx(); ror(); break;
	case 0x77: zpx(); rra(); break;
	case 0x78: imp(); sei(); break;
	case 0x79: absy(); adc(); break;
	case 0x7A: imp(); nop(); break;
	case 0x7B: absy(); rra(); break;
	case 0x7C: absx(); nop(); break;
	case 0x7D: absx(); adc(); break;
	case 0x7E: absx(); ror(); break;
	case 0x7F: absx(); rra(); break;
	case 0x80: imm(); nop(); break;
	case 0x81: indx(); sta(); break;
	case 0x82: imm(); nop(); break;
	case 0x83: indx(); sax(); break;
	case 0x84: zp(); sty(); break;
	case 0x85: zp(); sta(); break;
	case 0x86: zp(); stx(); break;
	case 0x87: zp(); sax(); break;
	case 0x88: imp(); dey(); break;
	case 0x89: imm(); nop(); break;
	case 0x8A: imp(); txa(); break;
	case 0x8B: imm(); nop(); break;
	case 0x8C: abso(); sty(); break;
	case 0x8D: abso(); sta(); break;
	case 0x8E: abso(); stx(); break;
	case 0x8F: abso(); sax(); break;
	case 0x90: rel(); bcc(); break;
	case 0x91: indy(); sta(); break;
	case 0x92: imp(); nop(); break;
	case 0x93: indy(); nop(); break;
	case 0x94: zpx(); sty(); break;
	case 0x95: zpx(); sta(); break;
	case 0x96: zpy(); stx(); break;
	case 0x97: zpy(); sax(); break;
	case 0x98: imp(); tya(); break;
	case 0x99: absy(); sta(); break;
	case 0x9A: imp(); txs(); break;
	case 0x9B: absy(); nop(); break;
	case 0x9C: absx(); nop(); break;
	case 0x9D: absx(); sta(); break;
	case 0x9E: absy(); nop(); break;
	case 0x9F: absy(); nop(); break;
	case 0xA0: imm(); ldy(); break;
	case 0xA1: indx(); lda(); break;
	case 0xA2: imm(); ldx(); break;
	case 0xA3: indx(); lax(); break;
	case 0xA4: zp(); ldy(); break;
	case 0xA5: zp(); lda(); break;
	case 0xA6: zp(); ldx(); break;
	case 0xA7: zp(); lax(); break;
	case 0xA8: imp(); tay(); break;
	case 0xA9: imm(); lda(); break;
	case 0xAA: imp(); tax(); break;
	case 0xAB: imm(); nop(); break;
	case 0xAC: abso(); ldy(); break;
	case 0xAD: abso(); lda(); break;
	case 0xAE: abso(); ldx(); break;
	case 0xAF: abso(); lax(); break;
	case 0xB0: rel(); bcs(); break;
	case 0xB1: indy(); lda(); break;
	case 0xB2: imp(); nop(); break;
	case 0xB3: indy(); lax(); break;
	case 0xB4: zpx(); ldy(); break;
	case 0xB5: zpx(); lda(); break;
	case 0xB6: zpy(); ldx(); break;
	case 0xB7: zpy(); lax(); break;
	case 0xB8: imp(); clv(); break;
	case 0xB9: absy(); lda(); break;
	case 0xBA: imp(); tsx(); break;
	case 0xBB: absy(); lax(); break;
	case 0xBC: absx(); ldy(); break;
	case 0xBD: absx(); lda(); break;
	case 0xBE: absy(); ldx(); break;
	case 0xBF: absy(); lax(); break;
	case 0xC0: imm(); cpy(); break;
	case 0xC1: indx(); cmp(); break;
	case 0xC2: imm(); nop(); break;
	case 0xC3: indx(); dcp(); break;
	case 0xC4: zp(); cpy(); break;
	case 0xC5: zp(); cmp(); break;
	case 0xC6: zp(); dec(); break;
	case 0xC7: zp(); dcp(); break;
	case 0xC8: imp(); iny(); break;
	case 0xC9: imm(); cmp(); break;
	case 0xCA: imp(); dex(); break;
	case 0xCB: imm(); nop(); break;
	case 0xCC: abso(); cpy(); break;
	case 0xCD: abso(); cmp(); break;
	case 0xCE: abso(); dec(); break;
	case 0xCF: abso(); dcp(); break;
	case 0xD0: rel(); bne(); break;
	case 0xD1: indy(); cmp(); break;
	case 0xD2: imp(); nop(); break;
	case 0xD3: indy(); dcp(); break;
	case 0xD4: zpx(); nop(); break;
	case 0xD5: zpx(); cmp(); break;
	case 0xD6: zpx(); dec(); break;
	case 0xD7: zpx(); dcp(); break;
	case 0xD8: imp(); cld(); break;
	case 0xD9: absy(); cmp(); break;
	case 0xDA: imp(); nop(); break;
	case 0xDB: absy(); dcp(); break;
	case 0xDC: absx(); nop(); break;
	case 0xDD: absx(); cmp(); break;
	case 0xDE: absx(); dec(); break;
	case 0xDF: absx(); dcp(); break;
	case 0xE0: imm(); cpx(); break;
	case 0xE1: indx(); sbc(); break;
	case 0xE2: imm(); nop(); break;
	case 0xE3: indx(); isb(); break;
	case 0xE4: zp(); cpx(); break;
	case 0xE5: zp(); sbc(); break;
	case 0xE6: zp(); inc(); break;
	case 0xE7: zp(); isb(); break;
	case 0xE8: imp(); inx(); break;
	case 0xE9: imm(); sbc(); break;
	case 0xEA: imp(); nop(); break;
	case 0xEB: imm(); sbc(); break;
	case 0xEC: abso(); cpx(); break;
	case 0xED: abso(); sbc(); break;
	case 0xEE: abso(); inc(); break;
	case 0xEF: abso(); isb(); break;
	case 0xF0: rel(); beq(); break;
	case 0xF1: indy(); sbc(); break;
	case 0xF2: imp(); nop(); break;
	case 0xF3: indy(); isb(); break;
	case 0xF4: zpx(); nop(); break;
	case 0xF5: zpx(); sbc(); break;
	case 0xF6: zpx(); inc(); break;
	case 0xF7: zpx(); isb(); break;
	case 0xF8: imp(); sed(); break;
	case 0xF9: absy(); sbc(); break;
	case 0xFA: imp(); nop(); break;
	case 0xFB: absy(); isb(); break;
	case 0xFC: absx(); nop(); break;
	case 0xFD: absx(); sbc(); break;
	case 0xFE: absx(); inc(); break;
	case 0xFF: absx(); isb(); break;
	}
}

// void Fake6502Wrapper::exec(/*uint32_t tickcount */) {
    // clockgoal6502 += tickcount;
   
    // while (/* clockticks6502 < clockgoal6502 */ true) {
        // m_opcode = read6502(m_pc++);
        // m_status |= FLAG_CONSTANT;

        // penaltyop = 0;
        // penaltyaddr = 0;

        // (*addrtable[opcode])();
        // (*optable[opcode])();

        // clockticks6502 += ticktable[opcode];
        // if (penaltyop && penaltyaddr) clockticks6502++;

        // instructions++;

        // if (callexternal) (*loopexternal)();
    // }
// }

void Fake6502Wrapper::step() {
    m_opcode = read6502(m_pc++);
    m_status |= FLAG_CONSTANT;

    // penaltyop = 0;
    // penaltyaddr = 0;

    // (*addrtable[opcode])();
    // (*optable[opcode])();
	dispatch();
    // clockticks6502 += ticktable[opcode];
    //if (penaltyop && penaltyaddr) clockticks6502++;
    // clockgoal6502 = clockticks6502;

    // instructions++;

    // if (callexternal) (*loopexternal)();
}

// void hookexternal(void *funcptr) {
//     if (funcptr != (void *)NULL) {
//         loopexternal = (void(*)())funcptr;
//         callexternal = 1;
//     } else callexternal = 0;
// }

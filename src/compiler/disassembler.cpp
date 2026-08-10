/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "assembler.h"
#include "disassembler.h"
#include "../utils/text.h"

/*
** {===========================================================================
** Disassembler
*/

namespace GBBASIC {

class DisassemblerImpl : public Disassembler {
public:
	DisassemblerImpl() {
	}
	virtual ~DisassemblerImpl() override {
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Object** ptr) const override { // Non-clonable.
		if (ptr)
			*ptr = nullptr;

		return false;
	}

	virtual bool disassemble(Mnemonic::Array &mnemonics, const Bytes::Ptr &bytes, const DisassemblingOptions &options) const override {
		// Prepare.
		mnemonics.clear();

		if (!bytes)
			return true;

		// Processors.
		auto formatDataByte = [] (UInt8 value) -> std::string {
			return "db 0x" + Text::toHex(value, 2, '0', false);
		};
		auto substituteOperand = [] (const char* mnemonic, const Bytes* operands, int offset) -> std::string {
			std::string txt = mnemonic;
			if (txt.find("n16") != std::string::npos) {
				const UInt16 value = (UInt16)operands->get(offset) | ((UInt16)operands->get(offset + 1) << 8);

				return Text::replace(txt, "n16", "0x" + Text::toHex((UInt32)value, 4, '0', false));
			}
			if (txt.find("a16") != std::string::npos) {
				const UInt16 value = (UInt16)operands->get(offset) | ((UInt16)operands->get(offset + 1) << 8);

				return Text::replace(txt, "a16", "0x" + Text::toHex((UInt32)value, 4, '0', false));
			}
			if (txt.find("n8") != std::string::npos) {
				const UInt8 value = operands->get(offset);

				return Text::replace(txt, "n8", "0x" + Text::toHex((UInt32)value, 2, '0', false));
			}
			if (txt.find("a8") != std::string::npos) {
				const UInt8 value = operands->get(offset);

				return Text::replace(txt, "a8", "0x" + Text::toHex((UInt32)value, 2, '0', false));
			}
			if (txt.find("e8") != std::string::npos) {
				const UInt8 value = operands->get(offset);

				return Text::replace(txt, "e8", "0x" + Text::toHex((UInt32)value, 2, '0', false));
			}

			return txt;
		};

		// Disassemble.
		const int size = (int)bytes->count();
		const int base = (options.bank == 0) ? 0 : options.startAddress;
		int bank = options.bank;
		int address = base + options.addressCursor;

		int offset = 0;
		while (offset < size) {
			const UInt8 opcode = bytes->get(offset);

			std::string text;
			int instSize = 0;

			if (opcode == ASSEMBLER_OPCODE_CB) {
				// CB-prefixed instruction.
				if (offset + 1 >= size) {
					text = formatDataByte(opcode);
					instSize = 1;
				} else {
					const UInt8 cbbyte = bytes->get(offset + 1);
					const char* mnemonic = ASSEMBLER_CB_OPCODE_MNEMONICS[cbbyte];
					if (mnemonic) {
						text = mnemonic;
					} else {
						text = formatDataByte(cbbyte);
					}
					instSize = 2;
				}
			} else {
				const char* mnemonic = ASSEMBLER_OPCODE_MNEMONICS[opcode];
				if (!mnemonic) {
					// Invalid opcode.
					text = formatDataByte(opcode);
					instSize = 1;
				} else {
					instSize = ASSEMBLER_OPCODE_SIZE[opcode];
					bool serialized = false;
					if (opcode == ASSEMBLER_OPCODE_STOP) {
						if (offset + instSize + 1 <= size) {
							const UInt8 afterStop = bytes->get(offset + 1);
							if (afterStop == ASSEMBLER_OPCODE_NOP) { // Handle 0x00 after "stop".
								text = mnemonic;
								instSize = 2;
								serialized = true;
							}
						}
					}
					if (!serialized) {
						if (offset + instSize > size) {
							// Truncated instruction.
							text = formatDataByte(opcode);
							instSize = 1;
						} else {
							text = substituteOperand(mnemonic, bytes.get(), offset + 1);
						}
					}
				}
			}

			mnemonics.push_back(Mnemonic(text, (UInt8)bank, (UInt16)address));

			offset += instSize;
			address += instSize;
			if (address >= options.bankSize) {
				const std::div_t div = std::div(address, options.bankSize);
				bank += div.quot;
				address = div.rem;
				if (bank > 0)
					address += options.startAddress;
			}
		}

		// Finish.
		return true;
	}
};

Disassembler::Mnemonic::Mnemonic() {
}

Disassembler::Mnemonic::Mnemonic(const std::string &txt, UInt8 b, UInt16 addr) :
	text(txt),
	bank(b), address(addr)
{
}

Disassembler::DisassemblingOptions::DisassemblingOptions() {
}

Disassembler::DisassemblingOptions::DisassemblingOptions(int bankSize_, int startAddr, int b, int addr) :
	bankSize(bankSize_),
	startAddress(startAddr),
	bank(b), addressCursor(addr)
{
}

Disassembler* Disassembler::create(void) {
	DisassemblerImpl* result = new DisassemblerImpl();

	return result;
}

void Disassembler::destroy(Disassembler* ptr) {
	DisassemblerImpl* impl = static_cast<DisassemblerImpl*>(ptr);
	delete impl;
}

}

/* ===========================================================================} */

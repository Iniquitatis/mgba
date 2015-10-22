/* Copyright (c) 2013-2015 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "IOViewer.h"

#include "GameController.h"

#include <QFontDatabase>
#include <QGridLayout>
#include <QRadioButton>
#include <QSpinBox>

extern "C" {
#include "gba/io.h"
}

using namespace QGBA;


QList<IOViewer::RegisterDescription> IOViewer::s_registers;

const QList<IOViewer::RegisterDescription>& IOViewer::registerDescriptions() {
	if (!s_registers.isEmpty()) {
		return s_registers;
	}
	// 0x04000000: DISPCNT
	s_registers.append({
		{ tr("Background mode"), 0, 3, {
			tr("Mode 0: 4 tile layers"),
			tr("Mode 1: 2 tile layers + 1 rotated/scaled tile layer"),
			tr("Mode 2: 2 rotated/scaled tile layers"),
			tr("Mode 3: Full 15-bit bitmap"),
			tr("Mode 4: Full 8-bit bitmap"),
			tr("Mode 5: Small 15-bit bitmap"),
			QString(),
			QString()
		} },
		{ tr("CGB Mode"), 3, 1, true },
		{ tr("Frame select"), 4 },
		{ tr("Unlocked HBlank"), 5 },
		{ tr("Linear OBJ tile mapping"), 6 },
		{ tr("Force blank screen"), 7 },
		{ tr("Enable background 0"), 8 },
		{ tr("Enable background 1"), 9 },
		{ tr("Enable background 2"), 10 },
		{ tr("Enable background 3"), 11 },
		{ tr("Enable OBJ"), 12 },
		{ tr("Enable Window 0"), 13 },
		{ tr("Enable Window 1"), 14 },
		{ tr("Enable OBJ Window"), 15 },
	});
	// 0x04000002: Green swap (undocumented and unimplemented)
	s_registers.append(RegisterDescription());
	// 0x04000004: DISPSTAT
	s_registers.append({
		{ tr("Currently in VBlank"), 0, 1, true },
		{ tr("Currently in HBlank"), 1, 1, true },
		{ tr("Currently in VCounter"), 2, 1, true },
		{ tr("Enable VBlank IRQ generation"), 3 },
		{ tr("Enable HBlank IRQ generation"), 4 },
		{ tr("Enable VCounter IRQ generation"), 5 },
		{ tr("VCounter scanline"), 8, 8 },
	});
	// 0x04000006: VCOUNT
	s_registers.append({
		{ tr("Current scanline"), 0, 8, true },
	});
	// 0x04000008: BG0CNT
	s_registers.append({
		{ tr("Priority"), 0, 2 },
		{ tr("Tile data base (* 16kB)"), 2, 2 },
		{ tr("Enable mosaic"), 6 },
		{ tr("Enable 256-color"), 7 },
		{ tr("Tile map base (* 2kB)"), 8, 5 },
		{ tr("Background dimensions"), 14, 2 },
	});
	// 0x0400000A: BG1CNT
	s_registers.append({
		{ tr("Priority"), 0, 2 },
		{ tr("Tile data base (* 16kB)"), 2, 2 },
		{ tr("Enable mosaic"), 6 },
		{ tr("Enable 256-color"), 7 },
		{ tr("Tile map base (* 2kB)"), 8, 5 },
		{ tr("Background dimensions"), 14, 2 },
	});
	// 0x0400000C: BG2CNT
	s_registers.append({
		{ tr("Priority"), 0, 2 },
		{ tr("Tile data base (* 16kB)"), 2, 2 },
		{ tr("Enable mosaic"), 6 },
		{ tr("Enable 256-color"), 7 },
		{ tr("Tile map base (* 2kB)"), 8, 5 },
		{ tr("Overflow wraps"), 13 },
		{ tr("Background dimensions"), 14, 2 },
	});
	// 0x0400000E: BG3CNT
	s_registers.append({
		{ tr("Priority"), 0, 2 },
		{ tr("Tile data base (* 16kB)"), 2, 2 },
		{ tr("Enable mosaic"), 6 },
		{ tr("Enable 256-color"), 7 },
		{ tr("Tile map base (* 2kB)"), 8, 5 },
		{ tr("Overflow wraps"), 13 },
		{ tr("Background dimensions"), 14, 2 },
	});
	// 0x04000010: BG0HOFS
	s_registers.append({
		{ tr("Horizontal offset"), 0, 9 },
	});
	// 0x04000012: BG0VOFS
	s_registers.append({
		{ tr("Vertical offset"), 0, 9 },
	});
	// 0x04000014: BG1HOFS
	s_registers.append({
		{ tr("Horizontal offset"), 0, 9 },
	});
	// 0x04000016: BG1VOFS
	s_registers.append({
		{ tr("Vertical offset"), 0, 9 },
	});
	// 0x04000018: BG2HOFS
	s_registers.append({
		{ tr("Horizontal offset"), 0, 9 },
	});
	// 0x0400001A: BG2VOFS
	s_registers.append({
		{ tr("Vertical offset"), 0, 9 },
	});
	// 0x0400001C: BG3HOFS
	s_registers.append({
		{ tr("Horizontal offset"), 0, 9 },
	});
	// 0x0400001E: BG3VOFS
	s_registers.append({
		{ tr("Vertical offset"), 0, 9 },
	});
	return s_registers;
}

IOViewer::IOViewer(GameController* controller, QWidget* parent)
	: QDialog(parent)
	, m_controller(controller)
{
	m_ui.setupUi(this);

	for (unsigned i = 0; i < REG_MAX >> 1; ++i) {
		const char* reg = GBAIORegisterNames[i];
		if (!reg) {
			continue;
		}
		m_ui.regSelect->addItem("0x0400" + QString("%1: %2").arg(i << 1, 4, 16, QChar('0')).toUpper().arg(reg), i << 1);
	}

	const QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	m_ui.regValue->setFont(font);

	connect(m_ui.buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonPressed(QAbstractButton*)));
	connect(m_ui.buttonBox, SIGNAL(rejected()), this, SLOT(close()));
	connect(m_ui.regSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(selectRegister()));

	m_b[0] = m_ui.b0;
	m_b[1] = m_ui.b1;
	m_b[2] = m_ui.b2;
	m_b[3] = m_ui.b3;
	m_b[4] = m_ui.b4;
	m_b[5] = m_ui.b5;
	m_b[6] = m_ui.b6;
	m_b[7] = m_ui.b7;
	m_b[8] = m_ui.b8;
	m_b[9] = m_ui.b9;
	m_b[10] = m_ui.bA;
	m_b[11] = m_ui.bB;
	m_b[12] = m_ui.bC;
	m_b[13] = m_ui.bD;
	m_b[14] = m_ui.bE;
	m_b[15] = m_ui.bF;

	for (int i = 0; i < 16; ++i) {
		connect(m_b[i], SIGNAL(toggled(bool)), this, SLOT(bitFlipped()));
	}

	selectRegister(0);
}

void IOViewer::updateRegister() {
	m_value = 0;
	uint16_t value = 0;
	m_controller->threadInterrupt();
	if (m_controller->isLoaded()) {
		value = GBAIORead(m_controller->thread()->gba, m_register);
	}
	m_controller->threadContinue();

	for (int i = 0; i < 16; ++i) {
		m_b[i]->setChecked(value & (1 << i) ? Qt::Checked : Qt::Unchecked);
	}
	m_value = value;
}

void IOViewer::bitFlipped() {
	m_value = 0;
	for (int i = 0; i < 16; ++i) {
		m_value |= m_b[i]->isChecked() << i;
	}
	m_ui.regValue->setText("0x" + QString("%1").arg(m_value, 4, 16, QChar('0')).toUpper());
}

void IOViewer::writeback() {
	m_controller->threadInterrupt();
	if (m_controller->isLoaded()) {
		GBAIOWrite(m_controller->thread()->gba, m_register, m_value);
	}
	m_controller->threadContinue();
	updateRegister();
}

void IOViewer::selectRegister(unsigned address) {
	m_register = address;
	QGridLayout* box = static_cast<QGridLayout*>(m_ui.regDescription->layout());
	if (box) {
		// I can't believe there isn't a real way to do this...
		while (!box->isEmpty()) {
			QLayoutItem* item = box->takeAt(0);
			if (item->widget()) {
				delete item->widget();
			}
			delete item;
		}
	} else {
		box = new QGridLayout;
	}
	if (registerDescriptions().count() > address >> 1) {
		// TODO: Remove the check when done filling in register information
		const RegisterDescription& description = registerDescriptions().at(address >> 1);
		int i = 0;
		for (const RegisterItem& ri : description) {
			QLabel* label = new QLabel(ri.description);
			box->addWidget(label, i, 0);
			if (ri.size == 1) {
				QCheckBox* check = new QCheckBox;
				check->setEnabled(!ri.readonly);
				box->addWidget(check, i, 1, Qt::AlignRight);
				connect(check, SIGNAL(toggled(bool)), m_b[ri.start], SLOT(setChecked(bool)));
				connect(m_b[ri.start], SIGNAL(toggled(bool)), check, SLOT(setChecked(bool)));
			} else if (ri.items.empty()) {
				QSpinBox* sbox = new QSpinBox;
				sbox->setEnabled(!ri.readonly);
				sbox->setMaximum((1 << ri.size) - 1);
				box->addWidget(sbox, i, 1, Qt::AlignRight);
				for (int b = 0; b < ri.size; ++b) {
					connect(sbox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this, b, &ri](int v) {
						m_b[ri.start + b]->setChecked(v & (1 << b));
					});
					auto connection = connect(m_b[ri.start + b], &QCheckBox::toggled, [sbox, this, b](bool t) {
						int v = sbox->value() & ~(1 << b);
						v |= t << b;
						bool signalsBlocked = sbox->blockSignals(true);
						sbox->setValue(v);
						sbox->blockSignals(signalsBlocked);
					});
					connect(sbox, &QObject::destroyed, [connection, this, b, &ri]() {
						bool signalsBlocked = m_b[ri.start + b]->blockSignals(true);
						m_b[ri.start + b]->disconnect(connection);
						m_b[ri.start + b]->blockSignals(signalsBlocked);
					});
				}
			} else {
				QButtonGroup* group = new QButtonGroup(box);
				group->setExclusive(true);
				for (int o = 0; o < 1 << ri.size; ++o) {
					if (ri.items.at(o).isNull()) {
						continue;
					}
					++i;
					QRadioButton* button = new QRadioButton(ri.items.at(o));
					button->setEnabled(!ri.readonly);
					box->addWidget(button, i, 0, 1, 2, Qt::AlignLeft);
					group->addButton(button, o);
				}
				for (int b = 0; b < ri.size; ++b) {
					auto connection = connect(m_b[ri.start + b], &QCheckBox::toggled, [group, this, &ri](bool) {
						unsigned v = (m_value >> ri.start) & ((1 << ri.size) - 1);
						for (int i = 0; i < 1 << ri.size; ++i) {
							QAbstractButton* button = group->button(i);
							if (!button) {
								continue;
							}
							bool signalsBlocked = button->blockSignals(true);
							button->setChecked(i == v);
							button->blockSignals(signalsBlocked);
						}
					});
					connect(group, &QObject::destroyed, [connection, this, b, &ri]() {
						m_b[ri.start + b]->disconnect(connection);
					});
				}
				connect(group, static_cast<void (QButtonGroup::*)(int, bool)>(&QButtonGroup::buttonToggled), [this, &ri](int v) {
					for (int i = 0; i < ri.size; ++i) {
						bool signalsBlocked = m_b[ri.start + i]->blockSignals(true);
						m_b[ri.start + i]->setChecked(v & (1 << i));
						m_b[ri.start + i]->blockSignals(signalsBlocked);
					}
				});
			}
			++i;
		}
	}
	m_ui.regDescription->setLayout(box);
	updateRegister();
}

void IOViewer::selectRegister() {
	selectRegister(m_ui.regSelect->currentData().toUInt());
}

void IOViewer::buttonPressed(QAbstractButton* button) {
	switch (m_ui.buttonBox->standardButton(button)) {
	case QDialogButtonBox::Reset:
		updateRegister();
	 	break;
	case QDialogButtonBox::Apply:
	 	writeback();
	 	break;
	default:
		break;
	}
}
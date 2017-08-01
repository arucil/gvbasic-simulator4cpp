#include "var_edit_dlg.h"
#include <QLineEdit>
#include <QFormLayout>
#include <QBoxLayout>
#include <QPushButton>
#include <QValidator>
#include <QSpinBox>
#include <cstdint>

using namespace std;
using namespace gvbsim;


VarEditDialog::VarEditDialog(QWidget *parent, GVB::Single *val, Value::Type type)
   : QDialog(parent), m_val(val), m_type(type) {
   auto fmlayout = new QFormLayout;
   fmlayout->addRow(tr("新值"), m_edit = new QLineEdit(this));
   switch (type) {
   case Value::Type::INT:
      m_edit->setText(QString::number(val->ival));
      m_edit->setValidator(new QIntValidator(INT16_MIN, INT16_MAX, this));
      break;
   case Value::Type::REAL:
      m_edit->setText(QString::number(val->rval, 'G', 9));
      m_edit->setValidator(new QDoubleValidator(this));
      break;
   case Value::Type::STRING:
      m_edit->setText(QString::fromStdString(val->sval));
      break;
   }
   
   auto blayout = new QHBoxLayout;
   auto btnOK = new QPushButton(tr("确定"));
   blayout->addWidget(btnOK, 1, Qt::AlignRight);
   auto btnCn = new QPushButton(tr("取消"));
   blayout->addWidget(btnCn, 0);
   
   auto layout = new QVBoxLayout(this);
   layout->addLayout(fmlayout);
   layout->addLayout(blayout);
   
   connect(btnOK, &QPushButton::clicked, this, &VarEditDialog::edit);
   connect(btnCn, &QPushButton::clicked, this, &VarEditDialog::close);
}

void VarEditDialog::edit() {
   switch (m_type) {
   case Value::Type::INT:
      m_val->ival = m_edit->text().toInt();
      break;
   case Value::Type::REAL:
      m_val->rval = m_edit->text().toDouble();
      break;
   case Value::Type::STRING:
      m_val->sval = m_edit->text().toStdString();
      break;
   }
   close();
}


ArrEditDialog::ArrEditDialog(QWidget *parent, GVB::Array *arr, Value::Type type)
   : QDialog(parent), m_arr(arr), m_type(type) {
   auto hlayout = new QHBoxLayout;
   for (auto i : arr->bounds) {
      auto p = new QSpinBox(this);
      p->setRange(0, i - 1);
      p->setSingleStep(1);
      p->setValue(0);
      connect(p, SIGNAL(valueChanged(int)), this, SLOT(updateValue(int)));
      m_spins.push_back(p);
      hlayout->addWidget(p, 0);
   }
   
   hlayout->addWidget(m_edit = new QLineEdit(this), 1, Qt::AlignRight);
   switch (type) {
   case Value::Type::INT:
      m_edit->setValidator(new QIntValidator(INT16_MIN, INT16_MAX, this));
      break;
   case Value::Type::REAL:
      m_edit->setValidator(new QDoubleValidator(this));
      break;
   case Value::Type::STRING:
      break;
   }
   
   updateValue(0);
   
   auto blayout = new QHBoxLayout;
   auto btnOK = new QPushButton(tr("修改"));
   btnOK->setAutoDefault(false);
   btnOK->setDefault(false);
   blayout->addWidget(btnOK, 1, Qt::AlignRight);
   auto btnCn = new QPushButton(tr("关闭"));
   blayout->addWidget(btnCn, 0);
   
   auto layout = new QVBoxLayout(this);
   layout->addLayout(hlayout);
   layout->addLayout(blayout);
   
   connect(btnOK, &QPushButton::clicked, this, &ArrEditDialog::edit);
   connect(btnCn, &QPushButton::clicked, this, &ArrEditDialog::close);
}

void ArrEditDialog::updateValue(int) {
   switch (m_type) {
   case Value::Type::INT:
      m_edit->setText(QString::number(m_arr->vals[calcOffset()].ival));
      break;
   case Value::Type::REAL:
      m_edit->setText(QString::number(m_arr->vals[calcOffset()].rval, 'G', 9));
      break;
   case Value::Type::STRING:
      m_edit->setText(QString::fromStdString(m_arr->vals[calcOffset()].sval));
      break;
   }
}

unsigned ArrEditDialog::calcOffset() {
   unsigned off = m_spins[0]->value();
   for (size_t i = 1; i < m_spins.size(); ++i) {
      off = off * m_arr->bounds[i] + m_spins[i]->value();
   }
   return off;
}

void ArrEditDialog::edit() {
   switch (m_type) {
   case Value::Type::INT:
      m_arr->vals[calcOffset()].ival = m_edit->text().toInt();
      break;
   case Value::Type::REAL:
      m_arr->vals[calcOffset()].rval = m_edit->text().toDouble();
      break;
   case Value::Type::STRING:
      m_arr->vals[calcOffset()].sval = m_edit->text().toStdString();
      break;
   }
}

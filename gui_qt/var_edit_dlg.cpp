#include "var_edit_dlg.h"
#include <QLineEdit>
#include <QFormLayout>
#include <QBoxLayout>
#include <QPushButton>
#include <QValidator>
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

#ifndef VAR_EDIT_DLG_H
#define VAR_EDIT_DLG_H

#include <QDialog>
#include "../gvb/gvb.h"


class QLineEdit;


class VarEditDialog : public QDialog {
   Q_OBJECT
   
public:
   VarEditDialog(QWidget *parent, gvbsim::GVB::Single *val,
                 gvbsim::Value::Type type);
   
private slots:
   void edit();
   
private:
   gvbsim::GVB::Single *const m_val;
   QLineEdit *m_edit;
   const gvbsim::Value::Type m_type;
};

class ArrEditDialog : public QDialog {
   Q_OBJECT
   
public:
   ArrEditDialog(QWidget *parnt, gvbsim::GVB::Array *arr,
                 gvbsim::Value::Type type);
   
private slots:
   void edit();
   
private:
   gvbsim::GVB::Array *const m_arr;
   QLineEdit *m_edit;
   const gvbsim::Value::Type m_type;
};

#endif // VAR_EDIT_DLG_H

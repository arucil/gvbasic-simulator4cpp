#ifndef VAR_EDIT_DLG_H
#define VAR_EDIT_DLG_H

#include <QDialog>
#include "../gvb/gvb.h"


class QLineEdit;


class VarEditDialog : public QDialog {
   Q_OBJECT
   
public:
   VarEditDialog(QWidget *parent, gvbsim::GVB::Single *var,
                 gvbsim::Value::Type type);
   
private slots:
   void edit();
   
private:
   gvbsim::GVB::Single *const m_var;
   QLineEdit *m_edit;
   const gvbsim::Value::Type m_type;
};

#endif // VAR_EDIT_DLG_H

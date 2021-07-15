#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "selfdrive/ui/qt/widgets/keyboard.h"

class QDialogBase : public QDialog {
  Q_OBJECT

protected:
  QDialogBase(QWidget *parent);
  bool eventFilter(QObject *o, QEvent *e) override;  
};

class InputDialog : public QDialogBase {
  Q_OBJECT

public:
  explicit InputDialog(const QString &prompt_text, QWidget *parent);
  static QString getText(const QString &prompt, QWidget *parent, int minLength = -1, const QString &defaultText = "");
  QString text();
  void setMessage(const QString &message, bool clearInputField = true);
  void setMinLength(int length);
  void show();

private:
  int minLength;
  QLineEdit *line;
  Keyboard *k;
  QLabel *label;
  QVBoxLayout *main_layout;

public slots:
  int exec() override;

private slots:
  void handleInput(const QString &s);

signals:
  void cancel();
  void emitText(const QString &text);
};

class ConfirmationDialog : public QDialogBase {
  Q_OBJECT

public:
  explicit ConfirmationDialog(const QString &prompt_text, const QString &confirm_text,
                              const QString &cancel_text, QWidget* parent = 0);
  static bool alert(const QString &prompt_text, QWidget *parent = 0);
  static bool confirm(const QString &prompt_text, QWidget *parent = 0);

private:
  QLabel *prompt;
  QVBoxLayout *main_layout;

public slots:
  int exec() override;
};

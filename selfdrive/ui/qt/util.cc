#include "selfdrive/ui/qt/util.h"

#include <QApplication>
#include <QLayoutItem>
#include <QStyleOption>

#include "selfdrive/common/params.h"
#include "selfdrive/common/swaglog.h"
#include "selfdrive/hardware/hw.h"

QString getBrand() {
  return Params().getBool("Passive") ? "대시캠" : "오픈파일럿";
}

QString getBrandVersion() {
  return getBrand() + " v" + QString::fromStdString(Params().get("Version")).left(14).trimmed();
}

void configFont(QPainter &p, const QString &family, int size, const QString &style) {
  QFont f(family);
  f.setPixelSize(size);
  f.setStyleName(style);
  p.setFont(f);
}

void clearLayout(QLayout* layout) {
  while (QLayoutItem* item = layout->takeAt(0)) {
    if (QWidget* widget = item->widget()) {
      widget->deleteLater();
    }
    if (QLayout* childLayout = item->layout()) {
      clearLayout(childLayout);
    }
    delete item;
  }
}

QString timeAgo(const QDateTime &date) {
  int diff = date.secsTo(QDateTime::currentDateTime());

  QString s;
  if (diff < 60) {
    s = "지금";
  } else if (diff < 60 * 60) {
    int minutes = diff / 60;
    s = QString("%1분 전").arg(minutes);
  } else if (diff < 60 * 60 * 24) {
    int hours = diff / (60 * 60);
    s = QString("%1시간 전").arg(hours);
  } else if (diff < 3600 * 24 * 7) {
    int days = diff / (60 * 60 * 24);
    s = QString("%1일 전").arg(days);
  } else {
    s = date.date().toString();
  }

  return s;
}

void setQtSurfaceFormat() {
  QSurfaceFormat fmt;
#ifdef __APPLE__
  fmt.setVersion(3, 2);
  fmt.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
  fmt.setRenderableType(QSurfaceFormat::OpenGL);
#else
  fmt.setRenderableType(QSurfaceFormat::OpenGLES);
#endif
  QSurfaceFormat::setDefaultFormat(fmt);
}

void initApp() {
  Hardware::set_display_power(true);
  Hardware::set_brightness(65);
  setQtSurfaceFormat();
  if (Hardware::EON()) {
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  }
}

ClickableWidget::ClickableWidget(QWidget *parent) : QWidget(parent) { }

void ClickableWidget::mouseReleaseEvent(QMouseEvent *event) {
  emit clicked();
}

// Fix stylesheets
void ClickableWidget::paintEvent(QPaintEvent *) {
  QStyleOption opt;
  opt.init(this);
  QPainter p(this);
  style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}


void swagLogMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
  static std::map<QtMsgType, int> levels = {
    {QtMsgType::QtDebugMsg, 10},
    {QtMsgType::QtInfoMsg, 20},
    {QtMsgType::QtWarningMsg, 30},
    {QtMsgType::QtCriticalMsg, 40},
    {QtMsgType::QtSystemMsg, 40},
    {QtMsgType::QtFatalMsg, 50},
  };

  std::string file, function;
  if (context.file != nullptr) file = context.file;
  if (context.function != nullptr) function = context.function;

  auto bts = msg.toUtf8();
  cloudlog_e(levels[type], file.c_str(), context.line, function.c_str(), "%s", bts.constData());
}

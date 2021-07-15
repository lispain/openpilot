#pragma once

#include <QButtonGroup>
#include <QVBoxLayout>
#include <QWidget>

#include "selfdrive/ui/qt/offroad/wifiManager.h"
#include "selfdrive/ui/qt/widgets/input.h"
#include "selfdrive/ui/qt/widgets/ssh_keys.h"
#include "selfdrive/ui/qt/widgets/toggle.h"

class NetworkStrengthWidget : public QWidget {
  Q_OBJECT

public:
  explicit NetworkStrengthWidget(int strength, QWidget* parent = nullptr) : strength_(strength), QWidget(parent) { setFixedSize(100, 15); }

private:
  void paintEvent(QPaintEvent* event) override;
  int strength_ = 0;
};

class WifiUI : public QWidget {
  Q_OBJECT

public:
  explicit WifiUI(QWidget *parent = 0, WifiManager* wifi = 0);

private:
  WifiManager *wifi = nullptr;
  QVBoxLayout* main_layout;

signals:
  void connectToNetwork(const Network &n);

public slots:
  void refresh();
};

class AdvancedNetworking : public QWidget {
  Q_OBJECT
public:
  explicit AdvancedNetworking(QWidget* parent = 0, WifiManager* wifi = 0);

private:
  LabelControl* ipLabel;
  WifiManager* wifi = nullptr;

signals:
  void backPress();

public slots:
  void toggleTethering(bool enabled);
  void refresh();
};

class Networking : public QWidget {
  Q_OBJECT

public:
  explicit Networking(QWidget* parent = 0, bool show_advanced = true);

private:
  QStackedLayout* main_layout = nullptr; // nm_warning, wifiScreen, advanced
  QWidget* wifiScreen = nullptr;
  AdvancedNetworking* an = nullptr;
  bool show_advanced;

  WifiUI* wifiWidget;
  WifiManager* wifi = nullptr;

protected:
  void showEvent(QShowEvent* event) override;

public slots:
  void refresh();

private slots:
  void connectToNetwork(const Network &n);
  void wrongPassword(const QString &ssid);
};

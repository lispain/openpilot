#include "selfdrive/ui/qt/offroad/networking.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>

#include "selfdrive/ui/qt/widgets/scrollview.h"
#include "selfdrive/ui/qt/util.h"


void NetworkStrengthWidget::paintEvent(QPaintEvent* event) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);
  p.setPen(Qt::NoPen);
  const QColor gray(0x54, 0x54, 0x54);
  for (int i = 0, x = 0; i < 5; ++i) {
    p.setBrush(i < strength_ ? Qt::white : gray);
    p.drawEllipse(x, 0, 15, 15);
    x += 20;
  }
}

// Networking functions

Networking::Networking(QWidget* parent, bool show_advanced) : QWidget(parent), show_advanced(show_advanced) {
  main_layout = new QStackedLayout(this);

  wifi = new WifiManager(this);
  connect(wifi, &WifiManager::wrongPassword, this, &Networking::wrongPassword);
  connect(wifi, &WifiManager::refreshSignal, this, &Networking::refresh);

  QWidget* wifiScreen = new QWidget(this);
  QVBoxLayout* vlayout = new QVBoxLayout(wifiScreen);
  if (show_advanced) {
    QPushButton* advancedSettings = new QPushButton("추가옵션");
    advancedSettings->setObjectName("advancedBtn");
    advancedSettings->setStyleSheet("margin-right: 30px;");
    advancedSettings->setFixedSize(350, 100);
    connect(advancedSettings, &QPushButton::released, [=]() { main_layout->setCurrentWidget(an); });
    vlayout->addSpacing(10);
    vlayout->addWidget(advancedSettings, 0, Qt::AlignRight);
    vlayout->addSpacing(10);
  }

  wifiWidget = new WifiUI(this, wifi);
  wifiWidget->setObjectName("wifiWidget");
  connect(wifiWidget, &WifiUI::connectToNetwork, this, &Networking::connectToNetwork);
  vlayout->addWidget(new ScrollView(wifiWidget, this), 1);
  main_layout->addWidget(wifiScreen);

  an = new AdvancedNetworking(this, wifi);
  connect(an, &AdvancedNetworking::backPress, [=]() { main_layout->setCurrentWidget(wifiScreen); });
  main_layout->addWidget(an);

  setStyleSheet(R"(
    #wifiWidget > QPushButton, #back_btn, #advancedBtn {
      font-size: 50px;
      margin: 0px;
      padding: 15px;
      border-width: 0;
      border-radius: 30px;
      color: #dddddd;
      background-color: #444444;
    }
    #wifiWidget > QPushButton:disabled {
      color: #777777;
      background-color: #222222;
    }
  )");
  main_layout->setCurrentWidget(wifiScreen);
}

void Networking::refresh() {
  wifiWidget->refresh();
  an->refresh();
}

void Networking::connectToNetwork(const Network &n) {
  if (wifi->isKnownConnection(n.ssid)) {
    wifi->activateWifiConnection(n.ssid);
  } else if (n.security_type == SecurityType::OPEN) {
    wifi->connect(n);
  } else if (n.security_type == SecurityType::WPA) {
    QString pass = InputDialog::getText(n.ssid + "의 패스워드를 입력하세요", 8);
    if (!pass.isEmpty()) {
      wifi->connect(n, pass);
    }
  }
}

void Networking::wrongPassword(const QString &ssid) {
  for (Network n : wifi->seen_networks) {
    if (n.ssid == ssid) {
      QString pass = InputDialog::getText(n.ssid + " 잘못된 패스워드 입니다", 8);
      if (!pass.isEmpty()) {
        wifi->connect(n, pass);
      }
      return;
    }
  }
}

// AdvancedNetworking functions

AdvancedNetworking::AdvancedNetworking(QWidget* parent, WifiManager* wifi): QWidget(parent), wifi(wifi) {

  QVBoxLayout* main_layout = new QVBoxLayout(this);
  main_layout->setMargin(40);
  main_layout->setSpacing(20);

  // Back button
  QPushButton* back = new QPushButton("뒤로가기");
  back->setObjectName("back_btn");
  back->setFixedSize(500, 100);
  connect(back, &QPushButton::released, [=]() { emit backPress(); });
  main_layout->addWidget(back, 0, Qt::AlignLeft);

  // Enable tethering layout
  ToggleControl *tetheringToggle = new ToggleControl("테더링 활성화", "", "", wifi->isTetheringEnabled());
  main_layout->addWidget(tetheringToggle);
  QObject::connect(tetheringToggle, &ToggleControl::toggleFlipped, this, &AdvancedNetworking::toggleTethering);
  main_layout->addWidget(horizontal_line(), 0);

  // Change tethering password
  ButtonControl *editPasswordButton = new ButtonControl("테더링 패스워드", "변경");
  connect(editPasswordButton, &ButtonControl::released, [=]() {
    QString pass = InputDialog::getText("새로운 패스워드를 입력하세요", this, 8, wifi->getTetheringPassword());
    if (!pass.isEmpty()) {
      wifi->changeTetheringPassword(pass);
    }
  });
  main_layout->addWidget(editPasswordButton, 0);
  main_layout->addWidget(horizontal_line(), 0);

  // IP address
  ipLabel = new LabelControl("IP 주소", wifi->ipv4_address);
  main_layout->addWidget(ipLabel, 0);
  main_layout->addWidget(horizontal_line(), 0);

  // SSH keys
  main_layout->addWidget(new SshToggle());
  main_layout->addWidget(horizontal_line(), 0);
  main_layout->addWidget(new SshControl());

  main_layout->addStretch(1);
}

void AdvancedNetworking::refresh() {
  ipLabel->setText(wifi->ipv4_address);
  update();
}

void AdvancedNetworking::toggleTethering(bool enabled) {
  wifi->setTetheringEnabled(enabled);
}

// WifiUI functions

WifiUI::WifiUI(QWidget *parent, WifiManager* wifi) : QWidget(parent), wifi(wifi) {
  main_layout = new QVBoxLayout(this);

  // Scan on startup
  QLabel *scanning = new QLabel("네트워크 검색 중");
  scanning->setStyleSheet(R"(font-size: 65px;)");
  main_layout->addWidget(scanning, 0, Qt::AlignCenter);
  main_layout->setSpacing(25);
}

void WifiUI::refresh() {
  clearLayout(main_layout);

  int i = 0;
  for (Network &network : wifi->seen_networks) {
    QHBoxLayout *hlayout = new QHBoxLayout;

    QLabel *ssid_label = new QLabel(QString::fromUtf8(network.ssid));
    ssid_label->setStyleSheet("font-size: 55px;");
    hlayout->addWidget(ssid_label, 1, Qt::AlignLeft);

    if (wifi->isKnownConnection(network.ssid) && !wifi->isTetheringEnabled()) {
      QPushButton *forgetBtn = new QPushButton();
      QPixmap pix("../assets/offroad/icon_close.svg");

      forgetBtn->setIcon(QIcon(pix));
      forgetBtn->setIconSize(QSize(35, 35));
      forgetBtn->setStyleSheet("QPushButton { background-color: #E22C2C; }");
      forgetBtn->setFixedSize(100, 90);

      QObject::connect(forgetBtn, &QPushButton::released, [=]() {
        if (ConfirmationDialog::confirm(QString::fromUtf8(network.ssid) + " 저장된 네트워크를 지우시겠습니까?", this)) {
          wifi->forgetConnection(network.ssid);
        }
      });

      hlayout->addWidget(forgetBtn, 0, Qt::AlignRight);
    } else if (network.security_type == SecurityType::WPA) {
      QLabel *lockIcon = new QLabel();
      QPixmap pix("../assets/offroad/icon_lock_closed.svg");
      lockIcon->setPixmap(pix.scaledToWidth(35, Qt::SmoothTransformation));
      lockIcon->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
      lockIcon->setStyleSheet("QLabel { margin: 0px; padding-left: 15px; padding-right: 15px; }");

      hlayout->addWidget(lockIcon, 0, Qt::AlignRight);
    }

    // strength indicator
    unsigned int strength_scale = network.strength / 17;
    hlayout->addWidget(new NetworkStrengthWidget(strength_scale), 0, Qt::AlignRight);

    // connect button
    QPushButton* btn = new QPushButton(network.security_type == SecurityType::UNSUPPORTED ? "연결불가" : (network.connected == ConnectedType::CONNECTED ? "연결됨" : (network.connected == ConnectedType::CONNECTING ? "연결중" : "연결")));
    btn->setDisabled(network.connected == ConnectedType::CONNECTED || network.connected == ConnectedType::CONNECTING || network.security_type == SecurityType::UNSUPPORTED);
    btn->setFixedWidth(350);
    QObject::connect(btn, &QPushButton::clicked, this, [=]() { emit connectToNetwork(network); });

    hlayout->addWidget(btn, 0, Qt::AlignRight);
    main_layout->addLayout(hlayout, 1);
    // Don't add the last horizontal line
    if (i+1 < wifi->seen_networks.size()) {
      main_layout->addWidget(horizontal_line(), 0);
    }
    i++;
  }
  main_layout->addStretch(3);
}

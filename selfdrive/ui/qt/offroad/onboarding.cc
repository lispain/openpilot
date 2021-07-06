#include "selfdrive/ui/qt/offroad/onboarding.h"

#include <QLabel>
#include <QPainter>
#include <QQmlContext>
#include <QQuickWidget>
#include <QVBoxLayout>

#include "selfdrive/common/util.h"
#include "selfdrive/ui/qt/widgets/input.h"

void TrainingGuide::mouseReleaseEvent(QMouseEvent *e) {
  if (boundingRect[currentIndex].contains(e->x(), e->y())) {
    currentIndex += 1;
  } else if (currentIndex == (boundingRect.size() - 2) && boundingRect.last().contains(e->x(), e->y())) {
    currentIndex = 0;
  }

  if (currentIndex >= (boundingRect.size() - 1)) {
    emit completedTraining();
  } else {
    image.load(IMG_PATH + "step" + QString::number(currentIndex) + ".png");
    update();
  }
}

void TrainingGuide::showEvent(QShowEvent *event) {
  currentIndex = 0;
  image.load(IMG_PATH + "step0.png");
}

void TrainingGuide::paintEvent(QPaintEvent *event) {
  QPainter painter(this);

  QRect bg(0, 0, painter.device()->width(), painter.device()->height());
  QBrush bgBrush("#000000");
  painter.fillRect(bg, bgBrush);

  QRect rect(image.rect());
  rect.moveCenter(bg.center());
  painter.drawImage(rect.topLeft(), image);
}

void TermsPage::showEvent(QShowEvent *event) {
  // late init, building QML widget takes 200ms
  if (layout()) {
    return;
  }

  QVBoxLayout *main_layout = new QVBoxLayout(this);
  main_layout->setMargin(40);
  main_layout->setSpacing(40);

  QQuickWidget *text = new QQuickWidget(this);
  text->setResizeMode(QQuickWidget::SizeRootObjectToView);
  text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  text->setAttribute(Qt::WA_AlwaysStackOnTop);
  text->setClearColor(Qt::transparent);

  QString text_view = util::read_file("../assets/offroad/tc.html").c_str();
  text->rootContext()->setContextProperty("text_view", text_view);
  text->rootContext()->setContextProperty("font_size", 55);

  text->setSource(QUrl::fromLocalFile("qt/offroad/text_view.qml"));

  main_layout->addWidget(text);

  QObject *obj = (QObject*)text->rootObject();
  QObject::connect(obj, SIGNAL(qmlSignal()), SLOT(enableAccept()));

  QHBoxLayout* buttons = new QHBoxLayout;
  main_layout->addLayout(buttons);

  QPushButton *decline_btn = new QPushButton("거절");
  buttons->addWidget(decline_btn);
  QObject::connect(decline_btn, &QPushButton::released, this, &TermsPage::declinedTerms);

  buttons->addSpacing(50);

  accept_btn = new QPushButton("스크롤하여 진행");
  accept_btn->setEnabled(false);
  buttons->addWidget(accept_btn);
  QObject::connect(accept_btn, &QPushButton::released, this, &TermsPage::acceptedTerms);
}

void TermsPage::enableAccept() {
  accept_btn->setText("동의");
  accept_btn->setEnabled(true);
}

void DeclinePage::showEvent(QShowEvent *event) {
  if (layout()) {
    return;
  }

  QVBoxLayout *main_layout = new QVBoxLayout(this);
  main_layout->setMargin(40);
  main_layout->setSpacing(40);

  QLabel *text = new QLabel(this);
  text->setText("오픈파일럿 사용을 위해서는 약관 및 조건에 동의해야 합니다.");
  text->setStyleSheet(R"(font-size: 50px;)");
  main_layout->addWidget(text, 0, Qt::AlignCenter);

  QHBoxLayout* buttons = new QHBoxLayout;
  main_layout->addLayout(buttons);

  QPushButton *back_btn = new QPushButton("뒤로가기");
  buttons->addWidget(back_btn);
  buttons->addSpacing(50);

  QObject::connect(back_btn, &QPushButton::released, this, &DeclinePage::getBack);

  QPushButton *uninstall_btn = new QPushButton("거절, 오픈파일럿 제거");
  uninstall_btn->setStyleSheet("background-color: #E22C2C;");
  buttons->addWidget(uninstall_btn);

  QObject::connect(uninstall_btn, &QPushButton::released, [=]() {
    if (ConfirmationDialog::confirm("제거하시겠습니까?", this)) {
      Params().putBool("DoUninstall", true);
    }
  });
}

void OnboardingWindow::updateActiveScreen() {
  bool accepted_terms = params.get("HasAcceptedTerms") == current_terms_version;
  bool training_done = params.get("CompletedTrainingVersion") == current_training_version;
  if (!accepted_terms) {
    setCurrentIndex(0);
  } else if (!training_done && !params.getBool("Passive")) {
    setCurrentIndex(1);
  } else {
    emit onboardingDone();
  }
}

OnboardingWindow::OnboardingWindow(QWidget *parent) : QStackedWidget(parent) {
  current_terms_version = params.get("TermsVersion");
  current_training_version = params.get("TrainingVersion");

  TermsPage* terms = new TermsPage(this);
  addWidget(terms);
  connect(terms, &TermsPage::acceptedTerms, [=]() {
    Params().put("HasAcceptedTerms", current_terms_version);
    updateActiveScreen();
  });
  connect(terms, &TermsPage::declinedTerms, [=]() { setCurrentIndex(2); });

  TrainingGuide* tr = new TrainingGuide(this);
  addWidget(tr);
  connect(tr, &TrainingGuide::completedTraining, [=]() {
    Params().put("CompletedTrainingVersion", current_training_version);
    updateActiveScreen();
  });

  DeclinePage* declinePage = new DeclinePage(this);
  addWidget(declinePage);
  connect(declinePage, &DeclinePage::getBack, [=]() { updateActiveScreen(); });

  setStyleSheet(R"(
    * {
      color: white;
      background-color: black;
    }
    QPushButton {
      padding: 50px;
      font-size: 50px;
      border-radius: 10px;
      background-color: #292929;
    }
    QPushButton:disabled {
      color: #777777;
      background-color: #222222;
    }
  )");
}

void OnboardingWindow::showEvent(QShowEvent *event) {
  updateActiveScreen();
}

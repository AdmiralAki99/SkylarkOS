#ifndef NUDGE_PAD_HPP
#define NUDGE_PAD_HPP

#include <QWidget>
#include <QGridLayout>
#include <QToolButton>

class NudgePad: public QWidget{
    Q_OBJECT
    public:
        explicit NudgePad(QWidget* parent = nullptr);

        signals:
            void forwardClicked();
            void backwardClicked();
            void leftClicked();
            void rightClicked();
            void stopClicked();

    private:
        QGridLayout* layout_ = nullptr;
        QToolButton* forwardButton_ = nullptr;
        QToolButton* backwardButton_ = nullptr;
        QToolButton* leftButton_ = nullptr;
        QToolButton* rightButton_ = nullptr;
        QToolButton* stopButton_ = nullptr;

        static QToolButton* makeNudgeButton(const QString &glyph, QWidget *parent);
};

#endif // NUDGE_PAD_HPP

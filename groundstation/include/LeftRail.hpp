#ifndef LEFT_RAIL_HPP
#define LEFT_RAIL_HPP

#include <QWidget>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QString>

class LeftRail: public QWidget{
    Q_OBJECT
    public:
        explicit LeftRail(QWidget* parent = nullptr);

        signals:
            void takeoffClicked();
            void returnClicked();
            void pauseClicked();
            void armClicked();

    private:
        QVBoxLayout* layout_ = nullptr;
        QToolButton* takeoffButton_ = nullptr;
        QToolButton* returnButton_ = nullptr;
        QToolButton* pauseButton_ = nullptr;
        QToolButton* armButton_ = nullptr;

        static QToolButton* makeRailButton(const QString &glyph, const QString &label, QWidget *parent);
};


#endif

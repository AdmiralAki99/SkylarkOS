#ifndef LEFT_RAIL_HPP
#define LEFT_RAIL_HPP

#include <QWidget>
#include <QLabel>
#include <QPushButton>
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
        QPushButton* takeoffButton_ = nullptr;
        QPushButton* returnButton_ = nullptr;
        QPushButton* pauseButton_ = nullptr;
        QPushButton* armButton_ = nullptr;
};


#endif
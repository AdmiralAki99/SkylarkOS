#ifndef TOP_BAR_HPP
#define TOP_BAR_HPP

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QString>

class TopBar: public QWidget{
    Q_OBJECT
    public:
        explicit TopBar(QWidget* parent = nullptr);
        void setLinkQuality(int quality);
        void setSatellites(int satellites);
        void setJetsonTemp(int tempC);
        void setBattery(double percent);
        void setArmed(bool armed);
        void setConnected(bool connected);

        signals:
            void modeSelected(const QString& mode);
            void armToggled();

    private:
        QPushButton* armButton_ = nullptr;
        QLabel* linkLabel_ = nullptr;
        QLabel* satLabel_ = nullptr;
        QLabel* tempLabel_ = nullptr;
        QLabel* batteryLabel_ = nullptr;
        QLabel* titleLabel_ = nullptr;
        QLabel* vehicleLabel_ = nullptr;
        QHBoxLayout* layout_ = nullptr;
        
};

#endif // TOP_BAR_HPP
#ifndef MISSION_PANEL_HPP
#define MISSION_PANEL_HPP

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QString>

class WaypointModel;

class MissionPanel : public QWidget {
    Q_OBJECT
public:
    explicit MissionPanel(WaypointModel *waypointModel, QWidget *parent = nullptr);

signals:
    void uploadRequested();

private:
    WaypointModel *waypointModel_ = nullptr;

    QVBoxLayout *layout_ = nullptr;
    QWidget *headerRow_ = nullptr;
    QLabel *headerLabel_ = nullptr;
    QWidget *listContainer_ = nullptr;
    QVBoxLayout *listLayout_ = nullptr;
    QPushButton *addButton_ = nullptr;
    QPushButton *uploadButton_ = nullptr;

    void rebuildRows();
};

#endif

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

    bool isCollapsed() const { return collapsed_; }
    int collapsedHeight() const;
    int contentHeight() const;

signals:
    void uploadRequested();
    void collapsedChanged(bool collapsed);
    void contentChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    WaypointModel *waypointModel_ = nullptr;

    QVBoxLayout *layout_ = nullptr;
    QWidget *headerRow_ = nullptr;
    QLabel *headerLabel_ = nullptr;
    QLabel *caretLabel_ = nullptr;
    QWidget *bodyContainer_ = nullptr;
    QWidget *listContainer_ = nullptr;
    QVBoxLayout *listLayout_ = nullptr;
    QPushButton *addButton_ = nullptr;
    QPushButton *uploadButton_ = nullptr;
    bool collapsed_ = false;

    void rebuildRows();
    void toggleCollapsed();
};

#endif

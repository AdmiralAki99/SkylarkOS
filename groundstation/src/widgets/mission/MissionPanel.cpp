#include "MissionPanel.hpp"
#include "WaypointModel.hpp"

#include <QEvent>
#include <QMouseEvent>

MissionPanel::MissionPanel(WaypointModel *waypointModel, QWidget *parent)
    : QWidget(parent), waypointModel_(waypointModel) {
    setObjectName("missionPanel");
    setAttribute(Qt::WA_StyledBackground, true);

    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(0);

    headerRow_ = new QWidget(this);
    headerRow_->setCursor(Qt::PointingHandCursor);
    headerRow_->installEventFilter(this);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerRow_);
    headerLayout->setContentsMargins(14, 11, 14, 11);
    headerLabel_ = new QLabel(this);
    headerLabel_->setObjectName("panelHeader");
    headerLayout->addWidget(headerLabel_);
    headerLayout->addStretch();
    caretLabel_ = new QLabel(QChar(0x25BE), headerRow_);
    caretLabel_->setObjectName("panelHeader");
    headerLayout->addWidget(caretLabel_);
    headerRow_->setStyleSheet("border-bottom: 1px solid #1c242a;");
    layout_->addWidget(headerRow_);

    bodyContainer_ = new QWidget(this);
    QVBoxLayout *bodyLayout = new QVBoxLayout(bodyContainer_);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    listContainer_ = new QWidget(bodyContainer_);
    listLayout_ = new QVBoxLayout(listContainer_);
    listLayout_->setContentsMargins(6, 6, 6, 6);
    listLayout_->setSpacing(2);
    bodyLayout->addWidget(listContainer_);

    addButton_ = new QPushButton("+ ADD WAYPOINT", bodyContainer_);
    addButton_->setObjectName("addWaypointButton");
    connect(addButton_, &QPushButton::clicked, this, [this]{
        waypointModel_->addWaypoint(37.7749, -122.4194);
    });
    bodyLayout->addWidget(addButton_);

    uploadButton_ = new QPushButton("UPLOAD & START MISSION", bodyContainer_);
    uploadButton_->setObjectName("uploadButton");
    connect(uploadButton_, &QPushButton::clicked, this, &MissionPanel::uploadRequested);
    bodyLayout->addWidget(uploadButton_);

    layout_->addWidget(bodyContainer_);
    layout_->addStretch();

    connect(waypointModel_, &WaypointModel::waypointsChanged, this, &MissionPanel::rebuildRows);
    rebuildRows();
}

bool MissionPanel::eventFilter(QObject *watched, QEvent *event) {
    if (watched == headerRow_ && event->type() == QEvent::MouseButtonPress) {
        toggleCollapsed();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void MissionPanel::toggleCollapsed() {
    collapsed_ = !collapsed_;
    bodyContainer_->setVisible(!collapsed_);
    caretLabel_->setText(collapsed_ ? QChar(0x25B8) : QChar(0x25BE));
    emit collapsedChanged(collapsed_);
}

int MissionPanel::collapsedHeight() const {
    return headerRow_->sizeHint().height();
}

int MissionPanel::contentHeight() const {
    return layout_->sizeHint().height();
}

void MissionPanel::rebuildRows() {
    headerLabel_->setText(QString("MISSION · %1 WP").arg(waypointModel_->count()));

    QLayoutItem *item;
    while ((item = listLayout_->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    for (int i = 0; i < waypointModel_->count(); ++i) {
        const int index = i;
        QWidget *row = new QWidget(listContainer_);
        QHBoxLayout *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(8, 8, 8, 8);
        rowLayout->setSpacing(8);

        QLabel *badge = new QLabel(waypointModel_->labelAt(index), row);
        badge->setObjectName("waypointBadge");
        badge->setFixedSize(22, 22);
        badge->setAlignment(Qt::AlignCenter);
        rowLayout->addWidget(badge);

        QLineEdit *altitudeField = new QLineEdit(QString::number(waypointModel_->altitudeAt(index), 'f', 0), row);
        altitudeField->setObjectName("altitudeField");
        altitudeField->setFixedWidth(44);
        connect(altitudeField, &QLineEdit::editingFinished, this, [this, index, altitudeField]{
            waypointModel_->setAltitude(index, altitudeField->text().toDouble());
        });
        rowLayout->addWidget(altitudeField);

        QLabel *unitLabel = new QLabel("m alt", row);
        unitLabel->setObjectName("altitudeUnitLabel");
        rowLayout->addWidget(unitLabel);

        rowLayout->addStretch();

        QPushButton *removeButton = new QPushButton(QChar(0x00D7), row);
        removeButton->setObjectName("removeWaypointButton");
        removeButton->setFlat(true);
        connect(removeButton, &QPushButton::clicked, this, [this, index]{
            waypointModel_->removeWaypoint(index);
        });
        rowLayout->addWidget(removeButton);

        listLayout_->addWidget(row);
    }

    emit contentChanged();
}

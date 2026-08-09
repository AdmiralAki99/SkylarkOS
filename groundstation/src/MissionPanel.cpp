#include "MissionPanel.hpp"
#include "WaypointModel.hpp"

MissionPanel::MissionPanel(WaypointModel *waypointModel, QWidget *parent)
    : QWidget(parent), waypointModel_(waypointModel) {
    setStyleSheet("MissionPanel { background: rgba(10,14,17,0.85); border: 1px solid #1c242a; border-radius: 12px; }");

    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(0);

    headerRow_ = new QWidget(this);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerRow_);
    headerLayout->setContentsMargins(14, 11, 14, 11);
    headerLabel_ = new QLabel(this);
    headerLabel_->setStyleSheet("color: #8fa3b0; font-weight: 600; font-size: 11px; letter-spacing: 1px;");
    headerLayout->addWidget(headerLabel_);
    headerLayout->addStretch();
    headerRow_->setStyleSheet("border-bottom: 1px solid #1c242a;");
    layout_->addWidget(headerRow_);

    listContainer_ = new QWidget(this);
    listLayout_ = new QVBoxLayout(listContainer_);
    listLayout_->setContentsMargins(6, 6, 6, 6);
    listLayout_->setSpacing(2);
    layout_->addWidget(listContainer_);

    addButton_ = new QPushButton("+ ADD WAYPOINT", this);
    addButton_->setStyleSheet(
        "QPushButton { color: #8fa3b0; background: transparent; border: 1px dashed #2a333b;"
        " border-radius: 8px; padding: 9px 0; font-size: 11px; letter-spacing: 0.5px; margin: 6px; }"
        "QPushButton:hover { border-color: #5aa9ff; color: #5aa9ff; }"
    );
    connect(addButton_, &QPushButton::clicked, this, [this]{
        // Placeholder default until wired to the drone's position or map center.
        waypointModel_->addWaypoint(37.7749, -122.4194);
    });
    layout_->addWidget(addButton_);

    layout_->addStretch();

    uploadButton_ = new QPushButton("UPLOAD & START MISSION", this);
    uploadButton_->setStyleSheet(
        "QPushButton { background: #1a4f8f; color: white; border: none; border-radius: 7px;"
        " padding: 9px 0; font-size: 11.5px; font-weight: 600; margin: 10px 14px; }"
        "QPushButton:hover { background: #245fa8; }"
    );
    connect(uploadButton_, &QPushButton::clicked, this, &MissionPanel::uploadRequested);
    layout_->addWidget(uploadButton_);

    connect(waypointModel_, &WaypointModel::waypointsChanged, this, &MissionPanel::rebuildRows);
    rebuildRows();
}

void MissionPanel::rebuildRows() {
    headerLabel_->setText(QString("MISSION · %1 WP").arg(waypointModel_->count()));

    // Full rebuild on every change rather than diffing rows — the list is
    // small enough that this is negligible.
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
        badge->setFixedSize(22, 22);
        badge->setAlignment(Qt::AlignCenter);
        badge->setStyleSheet(
            "background: #1a4f8f; border: 1px solid #5aa9ff; border-radius: 11px;"
            " color: white; font-weight: 600; font-size: 10.5px;"
        );
        rowLayout->addWidget(badge);

        QLineEdit *altitudeField = new QLineEdit(QString::number(waypointModel_->altitudeAt(index), 'f', 0), row);
        altitudeField->setFixedWidth(44);
        altitudeField->setStyleSheet(
            "background: #0f1418; border: 1px solid #22292f; border-radius: 4px;"
            " color: #e7edf2; font-size: 11px; padding: 3px 4px;"
        );
        connect(altitudeField, &QLineEdit::editingFinished, this, [this, index, altitudeField]{
            waypointModel_->setAltitude(index, altitudeField->text().toDouble());
        });
        rowLayout->addWidget(altitudeField);

        QLabel *unitLabel = new QLabel("m alt", row);
        unitLabel->setStyleSheet("color: #5f707c; font-size: 10px;");
        rowLayout->addWidget(unitLabel);

        rowLayout->addStretch();

        QPushButton *removeButton = new QPushButton(QChar(0x00D7), row);
        removeButton->setFlat(true);
        removeButton->setStyleSheet("color: #5f707c; font-size: 14px; border: none; background: transparent;");
        connect(removeButton, &QPushButton::clicked, this, [this, index]{
            waypointModel_->removeWaypoint(index);
        });
        rowLayout->addWidget(removeButton);

        listLayout_->addWidget(row);
    }
}

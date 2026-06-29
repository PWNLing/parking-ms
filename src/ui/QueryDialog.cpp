// ui/QueryDialog.cpp
#include "QueryDialog.h"
#include <QIcon>
#include "services/QueryService.h"
#include "core/Logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include <QMessageBox>
#include <QDateTime>
#include <QColor>
#include <QBrush>

QueryDialog::QueryDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("车库信息查询");
    setWindowIcon(QIcon(":/icons/logo.svg"));
    resize(900, 600);

    auto* root = new QVBoxLayout(this);
    root->setSpacing(12);
    root->setContentsMargins(16, 16, 16, 16);

    // 查询条件
    auto* gpCond = new QGroupBox("查询条件", this);
    auto* condRow = new QHBoxLayout(gpCond);

    condRow->addWidget(new QLabel("车牌号:", this));
    plateEdit_ = new QLineEdit(this);
    plateEdit_->setPlaceholderText("模糊匹配，如 津B");
    condRow->addWidget(plateEdit_);

    condRow->addWidget(new QLabel("起始:", this));
    fromEdit_ = new QDateTimeEdit(QDateTime::currentDateTime().addDays(-7), this);
    fromEdit_->setDisplayFormat("yyyy-MM-dd HH:mm");
    fromEdit_->setCalendarPopup(true);
    condRow->addWidget(fromEdit_);

    condRow->addWidget(new QLabel("结束:", this));
    toEdit_ = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    toEdit_->setDisplayFormat("yyyy-MM-dd HH:mm");
    toEdit_->setCalendarPopup(true);
    condRow->addWidget(toEdit_);

    auto* btnQuery  = new QPushButton("查询", this);
    btnQuery->setObjectName("primaryBtn");
    auto* btnReset  = new QPushButton("重置", this);
    connect(btnQuery, &QPushButton::clicked, this, &QueryDialog::onQuery);
    connect(btnReset, &QPushButton::clicked, this, &QueryDialog::onReset);
    condRow->addWidget(btnQuery);
    condRow->addWidget(btnReset);
    root->addWidget(gpCond);

    // 表格
    table_ = new QTableWidget(this);
    table_->setColumnCount(7);
    table_->setHorizontalHeaderLabels({"ID","车牌号","入库时间","出库时间","费用","地点","单价"});
    // 车牌号列(第1列)可编辑，其他列只读
    table_->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setAlternatingRowColors(true);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->verticalHeader()->setVisible(false);
    // 设置最大高度滚动
    table_->setMaximumHeight(400);
    connect(table_, &QTableWidget::itemSelectionChanged, this, &QueryDialog::onSelectionChanged);
    connect(table_, &QTableWidget::itemChanged, this, &QueryDialog::onCellChanged);
    root->addWidget(table_, 1);

    // 底部操作栏
    auto* bottomRow = new QHBoxLayout;
    countLabel_ = new QLabel("共 0 条", this);
    countLabel_->setObjectName("captionLabel");
    bottomRow->addWidget(countLabel_);

    // 新增：保存修改后的车牌
    btnSavePlate_ = new QPushButton("保存车牌修改", this);
    btnSavePlate_->setObjectName("primaryBtn");
    btnSavePlate_->setEnabled(false);
    btnSavePlate_->setToolTip("双击车牌号单元格可编辑，修改后点击此按钮保存到数据库");
    connect(btnSavePlate_, &QPushButton::clicked, this, &QueryDialog::onSavePlate);
    bottomRow->addStretch();
    bottomRow->addWidget(btnSavePlate_);

    btnDelete_ = new QPushButton("删除选中记录", this);
    btnDelete_->setObjectName("dangerBtn");
    btnDelete_->setEnabled(false);
    btnDelete_->setToolTip("删除在场中记录会自动联动 P_now_count -1");
    connect(btnDelete_, &QPushButton::clicked, this, &QueryDialog::onDelete);
    bottomRow->addWidget(btnDelete_);

    auto* btnClose = new QPushButton("关闭", this);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    bottomRow->addWidget(btnClose);
    root->addLayout(bottomRow);

    loadAll();
}

void QueryDialog::loadAll() {
    loadRecords(QueryService().queryAll());
}

void QueryDialog::loadRecords(const std::vector<CarRecord>& records) {
    loading_ = true;  // 屏蔽 itemChanged 信号
    table_->setRowCount(static_cast<int>(records.size()));
    int row = 0;
    for (const auto& r : records) {
        // ID 列：只读
        auto* idItem = new QTableWidgetItem(QString::number(r.id));
        idItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        table_->setItem(row, 0, idItem);

        // 车牌号列：可编辑（核心改动）
        auto* plateItem = new QTableWidgetItem(r.licensePlate);
        plateItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        plateItem->setForeground(QBrush(QColor("#1B5E20")));  // 绿色突出可编辑
        table_->setItem(row, 1, plateItem);

        // 其他列：只读
        auto setRO = [](QTableWidgetItem* it) {
            it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        };
        auto* inItem = new QTableWidgetItem(r.checkInTime);    setRO(inItem);
        auto* outItem = new QTableWidgetItem(r.checkOutDisplay()); setRO(outItem);
        auto* feeItem = new QTableWidgetItem(r.feeDisplay());  setRO(feeItem);
        auto* locItem = new QTableWidgetItem(r.location);     setRO(locItem);
        auto* priceItem = new QTableWidgetItem(QString::number(r.unitPrice, 'f', 2)); setRO(priceItem);
        table_->setItem(row, 2, inItem);
        table_->setItem(row, 3, outItem);
        table_->setItem(row, 4, feeItem);
        table_->setItem(row, 5, locItem);
        table_->setItem(row, 6, priceItem);
        row++;
    }
    loading_ = false;
    countLabel_->setText(QString("共 %1 条").arg(records.size()));
    if (records.size() >= 1000) {
        countLabel_->setText(countLabel_->text() + " （仅显示最近 1000 条，请用筛选缩小范围）");
    }
}

void QueryDialog::onQuery() {
    QString plate = plateEdit_->text().trimmed();
    bool hasPlate = !plate.isEmpty();
    bool hasTime  = fromEdit_->dateTime().isValid() && toEdit_->dateTime().isValid()
                    && fromEdit_->dateTime() <= toEdit_->dateTime();

    std::vector<CarRecord> records;
    if (hasPlate && hasTime) {
        records = QueryService().queryByPlateAndTime(plate, fromEdit_->dateTime(), toEdit_->dateTime());
    } else if (hasPlate) {
        records = QueryService().queryByPlate(plate);
    } else if (hasTime) {
        records = QueryService().queryByTimeRange(fromEdit_->dateTime(), toEdit_->dateTime());
    } else {
        loadAll();
        return;
    }
    loadRecords(records);
}

void QueryDialog::onReset() {
    plateEdit_->clear();
    fromEdit_->setDateTime(QDateTime::currentDateTime().addDays(-7));
    toEdit_->setDateTime(QDateTime::currentDateTime());
    loadAll();
}

void QueryDialog::onSelectionChanged() {
    bool hasSelection = !table_->selectedItems().isEmpty();
    btnDelete_->setEnabled(hasSelection);
    btnSavePlate_->setEnabled(hasSelection);
}

void QueryDialog::onCellChanged(QTableWidgetItem* item) {
    // 加载数据时屏蔽（避免 setItem 触发误报）
    if (loading_) return;
    if (!item) return;

    int row = item->row();
    int col = item->column();
    // 只关注车牌列(第1列)的修改
    if (col != 1) return;

    // 修改后立即转大写
    QString text = item->text().trimmed().toUpper();
    if (text != item->text()) {
        loading_ = true;
        item->setText(text);
        loading_ = false;
    }

    // 高亮提示有未保存的修改（变橙色）
    item->setForeground(QBrush(QColor("#FF6F00")));
    btnSavePlate_->setEnabled(true);
    countLabel_->setText(countLabel_->text() + "  ⚠ 有未保存的修改");
}

void QueryDialog::onSavePlate() {
    int row = table_->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先选中一行");
        return;
    }
    int id = table_->item(row, 0)->text().toInt();
    QString newPlate = table_->item(row, 1)->text().trimmed().toUpper();

    if (newPlate.isEmpty()) {
        QMessageBox::warning(this, "提示", "车牌号不能为空");
        return;
    }

    QString err;
    if (QueryService().updatePlate(id, newPlate, &err)) {
        auto* plateItem = table_->item(row, 1);
        plateItem->setForeground(QBrush(QColor("#1B5E20")));
        QMessageBox::information(this, "保存成功",
            QString("车牌已更新为: %1").arg(newPlate));
        loadAll();
    } else {
        QMessageBox::warning(this, "保存失败", err);
    }
}

void QueryDialog::onDelete() {
    int row = table_->currentRow();
    if (row < 0) return;
    int id = table_->item(row, 0)->text().toInt();
    QString plate = table_->item(row, 1)->text();
    auto ret = QMessageBox::question(this, "确认删除",
        QString("确认删除车牌 %1 的记录？\n\n注意：若为在场中记录，将同步 P_now_count -1").arg(plate),
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    QString err;
    if (QueryService().deleteRecord(id, &err)) {
        loadAll();
    } else {
        QMessageBox::warning(this, "删除失败", err);
    }
}

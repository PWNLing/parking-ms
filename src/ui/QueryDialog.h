// ui/QueryDialog.h — 车库信息查询
#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QTableWidget>
#include <QLabel>
class QueryDialog : public QDialog {
    Q_OBJECT
public:
    explicit QueryDialog(QWidget* parent = nullptr);

private slots:
    void onQuery();
    void onReset();
    void onDelete();
    void onSavePlate();       // 保存修改后的车牌
    void onSelectionChanged();
    void onCellChanged(QTableWidgetItem* item);  // 监听单元格修改

private:
    QLineEdit*    plateEdit_  = nullptr;
    QDateTimeEdit* fromEdit_  = nullptr;
    QDateTimeEdit* toEdit_    = nullptr;
    QTableWidget* table_      = nullptr;
    QLabel*       countLabel_ = nullptr;
    QPushButton*  btnDelete_  = nullptr;
    QPushButton*  btnSavePlate_ = nullptr;

    bool loading_ = false;    // 加载数据时屏蔽 onCellChanged

    void loadRecords(const std::vector<struct CarRecord>& records);
    void loadAll();
};

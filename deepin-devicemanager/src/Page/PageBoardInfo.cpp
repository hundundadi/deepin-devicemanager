// 项目自身文件
#include "PageBoardInfo.h"

// Qt库文件
#include <QTableWidgetItem>
#include <QDebug>

// Dtk头文件
#include <DApplicationHelper>
#include <DPalette>
#include <DFontSizeManager>

// 其它头文件
#include <unistd.h>
#include "DeviceInfo.h"
#include "DeviceBios.h"
#include "TextBrowser.h"
#include "RichTextDelegate.h"
#include "PageTableWidget.h"
#include "DeviceManager/DeviceManager.h"
#include "MacroDefinition.h"

PageBoardInfo::PageBoardInfo(QWidget *parent)
    : PageSingleInfo(parent)
    , mp_ItemDelegate(new RichTextDelegate(this))
{
}

void PageBoardInfo::updateInfo(const QList<DeviceBaseInfo *> &lst)
{
    mp_Device = lst[0];
    clearContent();

    // 获取主板信息
    DeviceBaseInfo *board = nullptr;
    QList<DeviceBaseInfo *> lstOther;
    foreach (DeviceBaseInfo *info, lst) {
        DeviceBios *bios = dynamic_cast<DeviceBios *>(info);
        if (!bios) {continue;}

        // 判断是否是主板
        if (bios->isBoard()) {
            board = info;
        } else {
            lstOther.append(info);
        }
    }

    if (!board) {return;}

    // 获取主板信息并加载
    QList<QPair<QString, QString>> baseInfoMap = board->getBaseAttribs();
    QList<QPair<QString, QString>> otherInfoMap = board->getOtherAttribs();
    baseInfoMap = baseInfoMap + otherInfoMap;
    loadDeviceInfo(lstOther, baseInfoMap);
}

void PageBoardInfo::loadDeviceInfo(const QList<DeviceBaseInfo *> &devices, const QList<QPair<QString, QString>> &lst)
{
    if (lst.size() < 1) {
        return;
    }

    // 比较页面可显示的最大行数与主板信息,取小值
    int maxRow = this->height() / ROW_HEIGHT - 3;
    int limitSize = std::min(lst.size(), maxRow);

    if (mp_Content) {
        mp_Content->setLimitRow(limitSize);
    }

    // 表格所有行数应等于主板信息行+其他信息行
    int row = lst.size() + devices.size();

    // 设置主板信息行数,此接口目前仅在主板界面中使用,用来配合更多/收起按钮的使用
    setDeviceInfoNum(lst.size());
    mp_Content->setColumnAndRow(row + 1, 2);

    // 主板信息正常显示
    for (int i = 0; i < lst.size(); ++i) {
        QTableWidgetItem *itemFirst = new QTableWidgetItem(lst[i].first);
        mp_Content->setItem(i, 0, itemFirst);
        QTableWidgetItem *itemSecond = new QTableWidgetItem(lst[i].second);
        mp_Content->setItem(i, 1, itemSecond);
    }

    QList<QPair<QString, QString>> pairs;
    getOtherInfoPair(devices, pairs);

    // 其他信息使用富文本代理
    // 其他信息的Id是出去所有BIOS信息以外的信息,使用Richtext进行显示
    for (int i = lst.size(); i < row; ++i) {
        mp_Content->setItemDelegateForRow(i, mp_ItemDelegate);
        QTableWidgetItem *itemFirst = new QTableWidgetItem(pairs[i - lst.size()].first);
        mp_Content->setItem(i, 0, itemFirst);

        QTableWidgetItem *itemSecond = new QTableWidgetItem(pairs[i - lst.size()].second);
        mp_Content->setItem(i, 1, itemSecond);

        // 计算行高
        QFont font = DFontSizeManager::instance()->t8();
        QFontMetrics fm(font);
        int height = 0;
        QStringList strList = pairs[i - lst.size()].second.split("\n");
        int fontHeight = fm.boundingRect(pairs[i - lst.size()].second).height() + 2;

        // 根据行数增加行高
        foreach (const QString &str, strList) {
            QStringList lst = str.split(":");
            if (lst.size() == 2) {
                int width = fm.boundingRect(lst[0]).width();
                int num = width / 110;
                int num0 = width % 110;
                if (num0 == 0) {
                    num = num - 1;
                }
                if (num > 0)
                    height += num * fontHeight;
            }
            QStringList attris = str.split("  /  \t\t");
            height += attris.size() * fontHeight;
        }
        height += 20;
        mp_Content->setRowHeight(i, height);
    }
}

void PageBoardInfo::getOtherInfoPair(const QList<DeviceBaseInfo *> &lst, QList<QPair<QString, QString>> &lstPair)
{
    // 获取其他信息键值对
    foreach (DeviceBaseInfo *dev, lst) {
        DeviceBios *bios = dynamic_cast<DeviceBios *>(dev);
        if (!bios) {continue;}
        QPair<QString, QString> pair;
        pair.first = bios->name();
        getValueInfo(bios, pair);
        lstPair.append(pair);
    }
}

void PageBoardInfo::getValueInfo(DeviceBaseInfo *device, QPair<QString, QString> &pair)
{
    // 获取信息并保存为pair
    QList<QPair<QString, QString>> baseInfoMap = device->getBaseAttribs();
    QList<QPair<QString, QString>> otherInfoMap = device->getOtherAttribs();
    baseInfoMap = baseInfoMap + otherInfoMap;
    QList<QPair<QString, QString>>::iterator it = baseInfoMap.begin();
    for (; it != baseInfoMap.end(); ++it) {
        pair.second += (*it).first;
        pair.second += ":";
        pair.second += (*it).second;
        pair.second += "\n";
    }
    pair.second.replace(QRegExp("\n$"), "");
}

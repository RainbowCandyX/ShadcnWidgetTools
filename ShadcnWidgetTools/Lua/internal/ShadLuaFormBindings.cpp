#include <QAbstractItemModel>
#include <QBuffer>
#include <QByteArray>
#include <QColor>
#include <QHash>
#include <QIODevice>
#include <QImage>
#include <QKeySequence>
#include <QList>
#include <QMetaType>
#include <QModelIndex>
#include <QObject>
#include <QRegularExpression>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadAutoComplete.h"
#include "ShadButton.h"
#include "ShadButtonGroup.h"
#include "ShadCodeEditor.h"
#include "ShadColorPicker.h"
#include "ShadDef.h"
#include "ShadDoubleSpinBox.h"
#include "ShadField.h"
#include "ShadFieldGroup.h"
#include "ShadFieldSeparator.h"
#include "ShadFieldSet.h"
#include "ShadFileDropZone.h"
#include "ShadIconType.h"
#include "ShadInput.h"
#include "ShadInputGroup.h"
#include "ShadInputOtp.h"
#include "ShadKeySequenceEdit.h"
#include "ShadLabel.h"
#include "ShadLuaBinder.h"
#include "ShadLuaBindings.h"
#include "ShadLuaInclude.h"
#include "ShadLuaObject.h"
#include "ShadLuaStack.h"
#include "ShadPropertyGrid.h"
#include "ShadQuestionnaire.h"
#include "ShadRadioButton.h"
#include "ShadRadioGroup.h"
#include "ShadSignaturePad.h"
#include "ShadTextarea.h"
#include "ShadTimePicker.h"
#include "ShadToggle.h"
#include "ShadToggleGroup.h"
#include "ShadTransfer.h"
#include "ShadTreeSelect.h"
#include "ShadTreeView.h"

namespace
{
    int optionalInt(lua_State* L, int index, int defaultValue)
    {
        qint64 value = 0;
        return ShadLuaStack::getInteger(L, index, value) ? int(value) : defaultValue;
    }

    QString optionalString(lua_State* L, int index)
    {
        QString value;
        ShadLuaStack::getString(L, index, value);
        return value;
    }

    ShadIconType::IconName optionalIcon(lua_State* L, int index)
    {
        return static_cast<ShadIconType::IconName>(optionalInt(L, index, ShadIconType::None));
    }

    // 颜色写法与 ShadLuaStack::getColor 相同；index 必须是正的栈位置
    bool readColors(lua_State* L, int index, QList<QColor>& colors)
    {
        if (lua_type(L, index) != LUA_TTABLE)
        {
            return false;
        }
        const int length = ShadLuaStack::tableLength(L, index);
        colors.clear();
        colors.reserve(length);
        for (int position = 1; position <= length; ++position)
        {
            QColor color;
            QString text;
            lua_rawgeti(L, index, position);
            const bool isColor = ShadLuaStack::getColor(L, -1, color) || (ShadLuaStack::getString(L, -1, text) && text.isEmpty());
            lua_pop(L, 1);
            if (!isColor)
            {
                return false;
            }
            colors.append(color);
        }
        return true;
    }

    // 无效颜色用空字符串表示，避免数组中出现 nil
    void pushColors(lua_State* L, const QList<QColor>& colors)
    {
        lua_createtable(L, int(colors.size()), 0);
        for (int position = 0; position < colors.size(); ++position)
        {
            ShadLuaStack::pushString(L, colors.at(position).isValid() ? colors.at(position).name(QColor::HexRgb) : QString());
            lua_rawseti(L, -2, position + 1);
        }
    }

    // 节点路径为从根节点到该节点的文字数组
    QModelIndex indexFromPath(const QAbstractItemModel* model, const QStringList& path)
    {
        QModelIndex index;
        if (model == nullptr)
        {
            return index;
        }
        for (const QString& text : path)
        {
            QModelIndex child;
            const int rowCount = model->rowCount(index);
            for (int row = 0; row < rowCount && !child.isValid(); ++row)
            {
                const QModelIndex candidate = model->index(row, 0, index);
                if (candidate.data().toString() == text)
                {
                    child = candidate;
                }
            }
            if (!child.isValid())
            {
                return QModelIndex();
            }
            index = child;
        }
        return index;
    }

    QStringList pathOfIndex(QModelIndex index)
    {
        QStringList path;
        for (; index.isValid(); index = index.parent())
        {
            path.prepend(index.data().toString());
        }
        return path;
    }

    // 读取栈上的节点路径并找到对应节点
    bool readPathIndex(lua_State* L, int stackIndex, const QAbstractItemModel* model, QModelIndex& index)
    {
        QStringList path;
        if (!ShadLuaStack::getStringList(L, stackIndex, path))
        {
            return false;
        }
        index = indexFromPath(model, path);
        return index.isValid();
    }

    // Lua 整数是 qlonglong，属性表格按 int 处理
    QVariant propertyValue(const QVariant& value, int targetTypeId)
    {
        if (targetTypeId == QMetaType::QColor && value.typeId() == QMetaType::QString)
        {
            return QVariant::fromValue(QColor(value.toString()));
        }
        if (targetTypeId == QMetaType::Double && value.typeId() == QMetaType::LongLong)
        {
            return QVariant(value.toDouble());
        }
        if (value.typeId() == QMetaType::LongLong)
        {
            return QVariant(int(value.toLongLong()));
        }
        return value;
    }

    // 输入类组件，只注册构造函数与非属性方法
    void registerInputs()
    {
        ShadLuaClass<ShadDoubleSpinBox>("DoubleSpinBox").constructor(&ShadLuaBind::newWithParent<ShadDoubleSpinBox>);

        ShadLuaClass<ShadTimePicker> timePicker("TimePicker");
        timePicker.constructor(&ShadLuaBind::newWithParent<ShadTimePicker>);
        timePicker.method("clear", &ShadTimePicker::clear);
        timePicker.method("getDisplayText", &ShadTimePicker::getDisplayText);
        timePicker.method("showPopup", &ShadTimePicker::showPopup);
        timePicker.method("hidePopup", &ShadTimePicker::hidePopup);
        timePicker.method("isPopupVisible", &ShadTimePicker::isPopupVisible);

        // setGroups({3, 3})；setPattern(正则表达式)
        ShadLuaClass<ShadInputOtp> inputOtp("InputOtp");
        inputOtp.constructor(&ShadLuaBind::newWithParent<ShadInputOtp>);
        inputOtp.method("setGroups", &ShadInputOtp::setGroups);
        inputOtp.method("getGroups", &ShadInputOtp::getGroups);
        inputOtp.method("getMaxLength", &ShadInputOtp::getMaxLength);
        inputOtp.method("clear", &ShadInputOtp::clear);
        inputOtp.method("setPattern", +[](ShadInputOtp* self, QString pattern) { self->setPattern(QRegularExpression(pattern)); });
        inputOtp.method("getPattern", +[](ShadInputOtp* self) { return self->getPattern().pattern(); });

        // new(control, parent) 或 new(parent)
        ShadLuaClass<ShadInputGroup> inputGroup("InputGroup");
        inputGroup.constructor(&ShadLuaBind::newWithEnum<ShadInputGroup, ShadInputGroupType::Control>);
        inputGroup.method("getInput", &ShadInputGroup::getInput);
        inputGroup.method("getTextarea", &ShadInputGroup::getTextarea);
        inputGroup.method("addText", &ShadInputGroup::addText);
        inputGroup.method("addIcon", &ShadInputGroup::addIcon);
        // addButton(position, text, icon, size)
        inputGroup.rawMethod("addButton", [](lua_State* L, ShadInputGroup* self) -> int
        {
            ShadLuaObject::push(L, self->addButton(static_cast<ShadInputGroupType::AddonPosition>(optionalInt(L, 2, ShadInputGroupType::InlineEnd)), optionalString(L, 3), optionalIcon(L, 4), static_cast<ShadButtonType::Size>(optionalInt(L, 5, ShadButtonType::ExtraSmall))));
            return 1;
        });
        inputGroup.method("addWidget", &ShadInputGroup::addWidget);
        inputGroup.method("addStretch", &ShadInputGroup::addStretch);

        // new(sequence, parent) 或 new(parent)
        ShadLuaClass<ShadKeySequenceEdit> keySequenceEdit("KeySequenceEdit");
        keySequenceEdit.constructor([](lua_State* L) -> ShadKeySequenceEdit*
        {
            if (lua_type(L, 1) == LUA_TSTRING)
            {
                return new ShadKeySequenceEdit(QKeySequence(optionalString(L, 1), QKeySequence::PortableText), ShadLuaObject::get<QWidget>(L, 2));
            }
            return new ShadKeySequenceEdit(ShadLuaObject::get<QWidget>(L, 1));
        });
        keySequenceEdit.method("clear", &ShadKeySequenceEdit::clear);
        keySequenceEdit.method("isRecording", &ShadKeySequenceEdit::isRecording);

        // 基本颜色与自定义颜色都是颜色字符串数组
        ShadLuaClass<ShadColorPicker> colorPicker("ColorPicker");
        colorPicker.constructor(&ShadLuaBind::newWithParent<ShadColorPicker>);
        colorPicker.rawMethod("setPresetColors", [](lua_State* L, ShadColorPicker* self) -> int
        {
            QList<QColor> colors;
            if (!readColors(L, 2, colors))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            self->setPresetColors(colors);
            return 0;
        });
        colorPicker.rawMethod("getPresetColors", [](lua_State* L, ShadColorPicker* self) -> int
        {
            pushColors(L, self->getPresetColors());
            return 1;
        });
        colorPicker.rawMethod("setCustomColors", [](lua_State* L, ShadColorPicker* self) -> int
        {
            QList<QColor> colors;
            if (!readColors(L, 2, colors))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            self->setCustomColors(colors);
            return 0;
        });
        colorPicker.rawMethod("getCustomColors", [](lua_State* L, ShadColorPicker* self) -> int
        {
            pushColors(L, self->getCustomColors());
            return 1;
        });
        colorPicker.method("pickScreenColor", &ShadColorPicker::pickScreenColor);
        colorPicker.method("isScreenPickingSupported", +[](ShadColorPicker* self) { Q_UNUSED(self); return ShadColorPicker::isScreenPickingSupported(); });

        // new(text, parent)、new(icon, parent) 或 new(parent)
        ShadLuaClass<ShadToggle> toggle("Toggle");
        toggle.constructor([](lua_State* L) -> ShadToggle*
        {
            if (lua_type(L, 1) == LUA_TNUMBER)
            {
                return new ShadToggle(optionalIcon(L, 1), ShadLuaObject::get<QWidget>(L, 2));
            }
            return ShadLuaBind::newWithText<ShadToggle>(L);
        });
        toggle.method("setShadIcon", &ShadToggle::setShadIcon);
        toggle.method("getShadIcon", &ShadToggle::getShadIcon);

        // new(selectionMode, parent) 或 new(parent)
        ShadLuaClass<ShadToggleGroup> toggleGroup("ToggleGroup");
        toggleGroup.constructor(&ShadLuaBind::newWithEnum<ShadToggleGroup, ShadToggleGroupType::SelectionMode>);
        // addItem(text) 或 addItem(icon, text)，返回新增的切换按钮
        toggleGroup.rawMethod("addItem", [](lua_State* L, ShadToggleGroup* self) -> int
        {
            if (lua_type(L, 2) == LUA_TNUMBER)
            {
                ShadLuaObject::push(L, self->addItem(optionalIcon(L, 2), optionalString(L, 3)));
                return 1;
            }
            ShadLuaObject::push(L, self->addItem(optionalString(L, 2)));
            return 1;
        });
        toggleGroup.method("itemAt", &ShadToggleGroup::itemAt);
        toggleGroup.method("count", &ShadToggleGroup::count);
        toggleGroup.method("setItemChecked", &ShadToggleGroup::setItemChecked);
        toggleGroup.method("isItemChecked", &ShadToggleGroup::isItemChecked);
        toggleGroup.method("getCheckedIndexes", &ShadToggleGroup::getCheckedIndexes);

        // new(orientation, parent) 或 new(parent)
        ShadLuaClass<ShadRadioGroup> radioGroup("RadioGroup");
        radioGroup.constructor(&ShadLuaBind::newWithEnum<ShadRadioGroup, Qt::Orientation>);
        radioGroup.method("addItem", &ShadRadioGroup::addItem);
        radioGroup.method("addItems", &ShadRadioGroup::addItems);
        radioGroup.method("itemAt", &ShadRadioGroup::itemAt);
        radioGroup.method("count", &ShadRadioGroup::count);

        ShadLuaClass<ShadButtonGroupText> buttonGroupText("ButtonGroupText");
        buttonGroupText.constructor(&ShadLuaBind::newWithText<ShadButtonGroupText>);
        buttonGroupText.method("setShadIcon", &ShadButtonGroupText::setShadIcon);
        buttonGroupText.method("getShadIcon", &ShadButtonGroupText::getShadIcon);

        // new(orientation, parent) 或 new(parent)
        ShadLuaClass<ShadButtonGroup> buttonGroup("ButtonGroup");
        buttonGroup.constructor(&ShadLuaBind::newWithEnum<ShadButtonGroup, Qt::Orientation>);
        buttonGroup.method("addWidget", &ShadButtonGroup::addWidget);
        buttonGroup.method("insertWidget", &ShadButtonGroup::insertWidget);
        buttonGroup.method("removeWidget", &ShadButtonGroup::removeWidget);
        buttonGroup.method("addSeparator", &ShadButtonGroup::addSeparator);
        // addText(text, icon)
        buttonGroup.rawMethod("addText", [](lua_State* L, ShadButtonGroup* self) -> int
        {
            ShadLuaObject::push(L, self->addText(optionalString(L, 2), optionalIcon(L, 3)));
            return 1;
        });
        buttonGroup.method("count", &ShadButtonGroup::count);
        buttonGroup.method("widgetAt", &ShadButtonGroup::widgetAt);

        ShadLuaClass<ShadFileDropZone> fileDropZone("FileDropZone");
        fileDropZone.constructor(&ShadLuaBind::newWithParent<ShadFileDropZone>);
        fileDropZone.method("setShadIcon", &ShadFileDropZone::setShadIcon);
        fileDropZone.method("getShadIcon", &ShadFileDropZone::getShadIcon);

        ShadLuaClass<ShadSignaturePad> signaturePad("SignaturePad");
        signaturePad.constructor(&ShadLuaBind::newWithParent<ShadSignaturePad>);
        signaturePad.method("isEmpty", &ShadSignaturePad::isEmpty);
        signaturePad.method("strokeCount", &ShadSignaturePad::strokeCount);
        // toPngBase64(trimmed)
        signaturePad.rawMethod("toPngBase64", [](lua_State* L, ShadSignaturePad* self) -> int
        {
            QByteArray bytes;
            QBuffer buffer(&bytes);
            const bool isTrimmed = lua_isnoneornil(L, 2) || lua_toboolean(L, 2) != 0;
            if (self->isEmpty() || !buffer.open(QIODevice::WriteOnly) || !self->toImage(isTrimmed).save(&buffer, "PNG"))
            {
                lua_pushnil(L);
                return 1;
            }
            ShadLuaStack::pushString(L, QString::fromLatin1(bytes.toBase64()));
            return 1;
        });

        // new(code, parent) 或 new(parent)
        ShadLuaClass<ShadCodeEditor>("CodeEditor").constructor(&ShadLuaBind::newWithText<ShadCodeEditor>);

        ShadLuaClass<ShadAutoComplete> autoComplete("AutoComplete");
        autoComplete.constructor(&ShadLuaBind::newWithParent<ShadAutoComplete>);
        autoComplete.method("setModel", &ShadAutoComplete::setModel);
        autoComplete.method("model", &ShadAutoComplete::model);
        autoComplete.method("showPopup", &ShadAutoComplete::showPopup);
        autoComplete.method("hidePopup", &ShadAutoComplete::hidePopup);
        autoComplete.method("isPopupVisible", &ShadAutoComplete::isPopupVisible);
    }

    bool readTransferItems(const QVariant& value, QList<ShadTransferItem>& items)
    {
        items.clear();
        if (value.typeId() == QMetaType::QVariantMap && value.toMap().isEmpty())
        {
            return true;
        }
        if (value.typeId() != QMetaType::QVariantList)
        {
            return false;
        }
        const QVariantList list = value.toList();
        items.reserve(list.size());
        for (const QVariant& entry : list)
        {
            if (entry.typeId() != QMetaType::QVariantMap)
            {
                return false;
            }
            const QVariantMap map = entry.toMap();
            ShadTransferItem item;
            item.key = map.value(QStringLiteral("key")).toString();
            item.title = map.value(QStringLiteral("title")).toString();
            item.description = map.value(QStringLiteral("description")).toString();
            item.icon = static_cast<ShadIconType::IconName>(map.value(QStringLiteral("icon"), int(ShadIconType::None)).toInt());
            item.isDisabled = map.value(QStringLiteral("disabled")).toBool();
            items.append(item);
        }
        return true;
    }

    QVariantList transferItemsToVariant(const QList<ShadTransferItem>& items)
    {
        QVariantList list;
        list.reserve(items.size());
        for (const ShadTransferItem& item : items)
        {
            QVariantMap map;
            map.insert(QStringLiteral("key"), item.key);
            map.insert(QStringLiteral("title"), item.title);
            map.insert(QStringLiteral("description"), item.description);
            map.insert(QStringLiteral("icon"), int(item.icon));
            map.insert(QStringLiteral("disabled"), item.isDisabled);
            list.append(map);
        }
        return list;
    }

    void registerTransfer()
    {
        ShadLuaClass<ShadTransfer> transfer("Transfer");
        transfer.constructor(&ShadLuaBind::newWithParent<ShadTransfer>);
        transfer.rawMethod("setItems", [](lua_State* L, ShadTransfer* self) -> int
        {
            QList<ShadTransferItem> items;
            if (!readTransferItems(ShadLuaStack::getVariant(L, 2), items))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            self->setItems(items);
            return 0;
        });
        transfer.rawMethod("getItems", [](lua_State* L, ShadTransfer* self) -> int
        {
            ShadLuaStack::pushVariant(L, QVariant(transferItemsToVariant(self->getItems())));
            return 1;
        });
        transfer.method("setTitles", &ShadTransfer::setTitles);
        transfer.method("getSourceTitle", &ShadTransfer::getSourceTitle);
        transfer.method("getTargetTitle", &ShadTransfer::getTargetTitle);
        transfer.method("getSourceCheckedKeys", &ShadTransfer::getSourceCheckedKeys);
        transfer.method("getTargetCheckedKeys", &ShadTransfer::getTargetCheckedKeys);
    }

    // 树形下拉选择，节点用路径表示
    void registerTreeSelect()
    {
        // new(selectionMode, parent) 或 new(parent)
        ShadLuaClass<ShadTreeSelect> treeSelect("TreeSelect");
        treeSelect.constructor(&ShadLuaBind::newWithEnum<ShadTreeSelect, ShadComboBoxType::SelectionMode>);
        // setModel(model)，传 nil 恢复内部模型
        treeSelect.rawMethod("setModel", [](lua_State* L, ShadTreeSelect* self) -> int
        {
            QAbstractItemModel* model = ShadLuaObject::get<QAbstractItemModel>(L, 2);
            if (model == nullptr && !lua_isnoneornil(L, 2))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            // 没有父对象的模型交给组件管理，避免被回收后悬空
            if (model != nullptr && model->parent() == nullptr)
            {
                model->setParent(self);
            }
            self->setModel(model);
            return 0;
        });
        treeSelect.method("model", &ShadTreeSelect::model);
        treeSelect.method("treeView", &ShadTreeSelect::treeView);
        // addItem(text, parentPath)
        treeSelect.rawMethod("addItem", [](lua_State* L, ShadTreeSelect* self) -> int
        {
            QString text;
            if (!ShadLuaStack::getString(L, 2, text))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            QStandardItem* parent = nullptr;
            if (!lua_isnoneornil(L, 3))
            {
                QModelIndex parentIndex;
                QStandardItemModel* model = qobject_cast<QStandardItemModel*>(self->model());
                if (model == nullptr || !readPathIndex(L, 3, model, parentIndex))
                {
                    return ShadLuaStack::error(L, QStringLiteral("父节点的路径不正确或父节点不存在"));
                }
                parent = model->itemFromIndex(parentIndex);
            }
            QStandardItem* item = self->addItem(text, parent);
            if (item == nullptr)
            {
                return ShadLuaStack::error(L, QStringLiteral("使用自己的模型时不能调用 addItem，请直接修改模型"));
            }
            ShadLuaStack::pushStringList(L, pathOfIndex(item->index()));
            return 1;
        });
        treeSelect.method("clearItems", &ShadTreeSelect::clearItems);
        // setCurrentPath(path)
        treeSelect.rawMethod("setCurrentPath", [](lua_State* L, ShadTreeSelect* self) -> int
        {
            QStringList path;
            if (!lua_isnoneornil(L, 2) && !ShadLuaStack::getStringList(L, 2, path))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            const QModelIndex index = indexFromPath(self->model(), path);
            if (!path.isEmpty() && !index.isValid())
            {
                return ShadLuaStack::error(L, QStringLiteral("节点不存在"));
            }
            self->setCurrentIndex(index);
            return 0;
        });
        // currentPath()
        treeSelect.rawMethod("currentPath", [](lua_State* L, ShadTreeSelect* self) -> int
        {
            ShadLuaStack::pushStringList(L, pathOfIndex(self->getCurrentIndex()));
            return 1;
        });
        treeSelect.method("getCurrentText", &ShadTreeSelect::getCurrentText);
        // setPathChecked(path, checked) 与 isPathChecked(path)
        treeSelect.rawMethod("setPathChecked", [](lua_State* L, ShadTreeSelect* self) -> int
        {
            QModelIndex index;
            if (!readPathIndex(L, 2, self->model(), index))
            {
                return ShadLuaStack::error(L, QStringLiteral("节点的路径不正确或节点不存在"));
            }
            self->setIndexChecked(index, lua_toboolean(L, 3) != 0);
            return 0;
        });
        treeSelect.rawMethod("isPathChecked", [](lua_State* L, ShadTreeSelect* self) -> int
        {
            QModelIndex index;
            if (!readPathIndex(L, 2, self->model(), index))
            {
                return ShadLuaStack::error(L, QStringLiteral("节点的路径不正确或节点不存在"));
            }
            lua_pushboolean(L, self->isIndexChecked(index) ? 1 : 0);
            return 1;
        });
        // getCheckedPaths()；getCheckedTexts()
        treeSelect.rawMethod("getCheckedPaths", [](lua_State* L, ShadTreeSelect* self) -> int
        {
            const QList<QModelIndex> indexes = self->getCheckedIndexes();
            lua_createtable(L, int(indexes.size()), 0);
            for (int position = 0; position < indexes.size(); ++position)
            {
                ShadLuaStack::pushStringList(L, pathOfIndex(indexes.at(position)));
                lua_rawseti(L, -2, position + 1);
            }
            return 1;
        });
        treeSelect.method("getCheckedTexts", &ShadTreeSelect::getCheckedTexts);
        treeSelect.method("clearChecked", &ShadTreeSelect::clearChecked);
        treeSelect.method("showPopup", &ShadTreeSelect::showPopup);
        treeSelect.method("hidePopup", &ShadTreeSelect::hidePopup);
        treeSelect.method("isPopupVisible", &ShadTreeSelect::isPopupVisible);
    }

    // 表单类组件
    void registerForms()
    {
        // new(label, control, parent) 或 new(parent)
        ShadLuaClass<ShadField> field("Field");
        field.constructor([](lua_State* L) -> ShadField*
        {
            if (lua_type(L, 1) == LUA_TSTRING)
            {
                return new ShadField(optionalString(L, 1), ShadLuaObject::get<QWidget>(L, 2), ShadLuaObject::get<QWidget>(L, 3));
            }
            return new ShadField(ShadLuaObject::get<QWidget>(L, 1));
        });
        field.method("labelWidget", &ShadField::labelWidget);
        field.method("setControl", &ShadField::setControl);
        field.method("getControl", &ShadField::getControl);
        field.method("setErrors", &ShadField::setErrors);
        field.method("getErrors", &ShadField::getErrors);
        field.method("setError", &ShadField::setError);
        field.method("clearErrors", &ShadField::clearErrors);

        ShadLuaClass<ShadFieldSeparator>("FieldSeparator").constructor(&ShadLuaBind::newWithText<ShadFieldSeparator>);

        ShadLuaClass<ShadFieldGroup> fieldGroup("FieldGroup");
        fieldGroup.constructor(&ShadLuaBind::newWithParent<ShadFieldGroup>);
        fieldGroup.method("addWidget", &ShadFieldGroup::addWidget);
        // addSeparator(text)
        fieldGroup.rawMethod("addSeparator", [](lua_State* L, ShadFieldGroup* self) -> int
        {
            ShadLuaObject::push(L, self->addSeparator(optionalString(L, 2)));
            return 1;
        });
        fieldGroup.method("removeWidget", &ShadFieldGroup::removeWidget);

        // new(legend, description, parent) 或 new(parent)
        ShadLuaClass<ShadFieldSet> fieldSet("FieldSet");
        fieldSet.constructor([](lua_State* L) -> ShadFieldSet*
        {
            if (lua_type(L, 1) == LUA_TSTRING)
            {
                return new ShadFieldSet(optionalString(L, 1), optionalString(L, 2), ShadLuaObject::get<QWidget>(L, 3));
            }
            return new ShadFieldSet(ShadLuaObject::get<QWidget>(L, 1));
        });
        fieldSet.method("addWidget", &ShadFieldSet::addWidget);
        fieldSet.method("contentLayout", &ShadFieldSet::contentLayout);

        // 题目与选项都用名称标识
        ShadLuaClass<ShadQuestionnaire> questionnaire("Questionnaire");
        questionnaire.constructor(&ShadLuaBind::newWithParent<ShadQuestionnaire>);
        // addItem(name, title, description)
        questionnaire.rawMethod("addItem", [](lua_State* L, ShadQuestionnaire* self) -> int
        {
            self->addItem(optionalString(L, 2), optionalString(L, 3), optionalString(L, 4));
            return 0;
        });
        questionnaire.method("getItems", &ShadQuestionnaire::getItems);
        questionnaire.method("setItemRequired", &ShadQuestionnaire::setItemRequired);
        questionnaire.method("isItemRequired", &ShadQuestionnaire::isItemRequired);
        questionnaire.method("setItemMultiple", &ShadQuestionnaire::setItemMultiple);
        questionnaire.method("isItemMultiple", &ShadQuestionnaire::isItemMultiple);
        questionnaire.method("setItemDisabled", &ShadQuestionnaire::setItemDisabled);
        questionnaire.method("isItemDisabled", &ShadQuestionnaire::isItemDisabled);
        questionnaire.method("setItemTitle", &ShadQuestionnaire::setItemTitle);
        questionnaire.method("setItemDescription", &ShadQuestionnaire::setItemDescription);
        // addChoice(name, value, label, description)
        questionnaire.rawMethod("addChoice", [](lua_State* L, ShadQuestionnaire* self) -> int
        {
            self->addChoice(optionalString(L, 2), optionalString(L, 3), optionalString(L, 4), optionalString(L, 5));
            return 0;
        });
        questionnaire.method("setChoiceDisabled", &ShadQuestionnaire::setChoiceDisabled);
        questionnaire.method("setChoiceLabel", &ShadQuestionnaire::setChoiceLabel);
        questionnaire.method("setChoiceDescription", &ShadQuestionnaire::setChoiceDescription);
        questionnaire.method("setItemInput", &ShadQuestionnaire::setItemInput);
        questionnaire.method("setPreviousText", &ShadQuestionnaire::setPreviousText);
        questionnaire.method("getPreviousText", &ShadQuestionnaire::getPreviousText);
        questionnaire.method("setSkipText", &ShadQuestionnaire::setSkipText);
        questionnaire.method("getSkipText", &ShadQuestionnaire::getSkipText);
        questionnaire.method("setNextText", &ShadQuestionnaire::setNextText);
        questionnaire.method("getNextText", &ShadQuestionnaire::getNextText);
        questionnaire.method("setSubmitText", &ShadQuestionnaire::setSubmitText);
        questionnaire.method("getSubmitText", &ShadQuestionnaire::getSubmitText);
        questionnaire.method("getCurrentIndex", &ShadQuestionnaire::getCurrentIndex);
        questionnaire.method("getItemCount", &ShadQuestionnaire::getItemCount);
        questionnaire.method("getItemStatus", &ShadQuestionnaire::getItemStatus);
        questionnaire.method("getAnswers", &ShadQuestionnaire::getAnswers);
        questionnaire.method("setAnswers", &ShadQuestionnaire::setAnswers);
        // getAllAnswers()
        questionnaire.rawMethod("getAllAnswers", [](lua_State* L, ShadQuestionnaire* self) -> int
        {
            const QHash<QString, QStringList> answers = self->getAllAnswers();
            lua_createtable(L, 0, int(answers.size()));
            for (auto it = answers.cbegin(); it != answers.cend(); ++it)
            {
                const QByteArray name = it.key().toUtf8();
                ShadLuaStack::pushStringList(L, it.value());
                lua_setfield(L, -2, name.constData());
            }
            return 1;
        });
        questionnaire.method("setItemError", &ShadQuestionnaire::setItemError);
        questionnaire.method("getItemError", &ShadQuestionnaire::getItemError);

        // 属性用 key 标识，值的类型决定编辑器
        ShadLuaClass<ShadPropertyGrid> propertyGrid("PropertyGrid");
        propertyGrid.constructor(&ShadLuaBind::newWithParent<ShadPropertyGrid>);
        // addProperty(key, label, value, group)
        propertyGrid.rawMethod("addProperty", [](lua_State* L, ShadPropertyGrid* self) -> int
        {
            lua_pushboolean(L, self->addProperty(optionalString(L, 2), optionalString(L, 3), propertyValue(ShadLuaStack::getVariant(L, 4), QMetaType::UnknownType), optionalString(L, 5)) ? 1 : 0);
            return 1;
        });
        // addDoubleProperty(key, label, value, group)
        propertyGrid.rawMethod("addDoubleProperty", [](lua_State* L, ShadPropertyGrid* self) -> int
        {
            double value = 0.0;
            if (!ShadLuaStack::getNumber(L, 4, value))
            {
                return ShadLuaStack::argumentError(L, 3);
            }
            lua_pushboolean(L, self->addProperty(optionalString(L, 2), optionalString(L, 3), QVariant(value), optionalString(L, 5)) ? 1 : 0);
            return 1;
        });
        // addColorProperty(key, label, color, group)
        propertyGrid.rawMethod("addColorProperty", [](lua_State* L, ShadPropertyGrid* self) -> int
        {
            QColor color;
            if (!ShadLuaStack::getColor(L, 4, color))
            {
                return ShadLuaStack::argumentError(L, 3);
            }
            lua_pushboolean(L, self->addProperty(optionalString(L, 2), optionalString(L, 3), QVariant::fromValue(color), optionalString(L, 5)) ? 1 : 0);
            return 1;
        });
        // addEnumProperty(key, label, options, currentIndex, group)
        propertyGrid.rawMethod("addEnumProperty", [](lua_State* L, ShadPropertyGrid* self) -> int
        {
            QStringList options;
            if (!ShadLuaStack::getStringList(L, 4, options))
            {
                return ShadLuaStack::argumentError(L, 3);
            }
            lua_pushboolean(L, self->addEnumProperty(optionalString(L, 2), optionalString(L, 3), options, optionalInt(L, 5, 0), optionalString(L, 6)) ? 1 : 0);
            return 1;
        });
        // addWidgetProperty(key, label, editor, group)
        propertyGrid.rawMethod("addWidgetProperty", [](lua_State* L, ShadPropertyGrid* self) -> int
        {
            lua_pushboolean(L, self->addWidgetProperty(optionalString(L, 2), optionalString(L, 3), ShadLuaObject::get<QWidget>(L, 4), optionalString(L, 5)) ? 1 : 0);
            return 1;
        });
        propertyGrid.method("removeProperty", &ShadPropertyGrid::removeProperty);
        propertyGrid.method("clear", &ShadPropertyGrid::clear);
        propertyGrid.method("hasProperty", &ShadPropertyGrid::hasProperty);
        propertyGrid.method("getKeys", &ShadPropertyGrid::getKeys);
        propertyGrid.method("setValue", +[](ShadPropertyGrid* self, QString key, QVariant value) { self->setValue(key, propertyValue(value, self->getValue(key).typeId())); });
        propertyGrid.method("getValue", &ShadPropertyGrid::getValue);
        propertyGrid.method("setPropertyLabel", &ShadPropertyGrid::setPropertyLabel);
        propertyGrid.method("getPropertyLabel", &ShadPropertyGrid::getPropertyLabel);
        propertyGrid.method("setPropertyDescription", &ShadPropertyGrid::setPropertyDescription);
        propertyGrid.method("getPropertyDescription", &ShadPropertyGrid::getPropertyDescription);
        propertyGrid.method("setPropertyReadOnly", &ShadPropertyGrid::setPropertyReadOnly);
        propertyGrid.method("isPropertyReadOnly", &ShadPropertyGrid::isPropertyReadOnly);
        propertyGrid.method("setPropertyRange", +[](ShadPropertyGrid* self, QString key, QVariant minimum, QVariant maximum) { self->setPropertyRange(key, propertyValue(minimum, self->getValue(key).typeId()), propertyValue(maximum, self->getValue(key).typeId())); });
        propertyGrid.method("setEnumOptions", &ShadPropertyGrid::setEnumOptions);
        propertyGrid.method("getEditor", &ShadPropertyGrid::getEditor);
        propertyGrid.method("setGroupTitle", &ShadPropertyGrid::setGroupTitle);
        propertyGrid.method("getGroupTitle", &ShadPropertyGrid::getGroupTitle);
        propertyGrid.method("setGroupExpanded", &ShadPropertyGrid::setGroupExpanded);
        propertyGrid.method("isGroupExpanded", &ShadPropertyGrid::isGroupExpanded);
        propertyGrid.method("getCurrentKey", &ShadPropertyGrid::getCurrentKey);
    }
} // namespace

namespace ShadLuaBindings
{
    void registerFormComponents()
    {
        registerInputs();
        registerTransfer();
        registerTreeSelect();
        registerForms();
    }
} // namespace ShadLuaBindings

#include <QAbstractItemModel>
#include <QAction>
#include <QBuffer>
#include <QByteArray>
#include <QColor>
#include <QIODevice>
#include <QImage>
#include <QKeySequence>
#include <QList>
#include <QMetaType>
#include <QModelIndex>
#include <QObject>
#include <QPixmap>
#include <QRect>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadAttachment.h"
#include "ShadAttachmentGroup.h"
#include "ShadAvatar.h"
#include "ShadAvatarGroup.h"
#include "ShadBreadcrumb.h"
#include "ShadButton.h"
#include "ShadDef.h"
#include "ShadDescriptions.h"
#include "ShadDropdownMenu.h"
#include "ShadEmpty.h"
#include "ShadIconType.h"
#include "ShadImageCropper.h"
#include "ShadImageViewer.h"
#include "ShadItem.h"
#include "ShadItemGroup.h"
#include "ShadKbd.h"
#include "ShadLabel.h"
#include "ShadListView.h"
#include "ShadLuaBinder.h"
#include "ShadLuaBindings.h"
#include "ShadLuaInclude.h"
#include "ShadLuaObject.h"
#include "ShadLuaStack.h"
#include "ShadMarkdownView.h"
#include "ShadTimeline.h"

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

    QKeySequence optionalKeySequence(lua_State* L, int index)
    {
        return QKeySequence(optionalString(L, index), QKeySequence::PortableText);
    }

    template <class T>
    void pushObjects(lua_State* L, const QList<T*>& objects)
    {
        lua_createtable(L, int(objects.size()), 0);
        for (int position = 0; position < objects.size(); ++position)
        {
            ShadLuaObject::push(L, objects.at(position));
            lua_rawseti(L, -2, position + 1);
        }
    }

    // new(icon, title, description, parent)、new(title, description, parent) 或 new(parent)
    template <class T>
    T* newWithIconTitle(lua_State* L)
    {
        if (lua_type(L, 1) == LUA_TNUMBER)
        {
            return new T(optionalIcon(L, 1), optionalString(L, 2), optionalString(L, 3), ShadLuaObject::get<QWidget>(L, 4));
        }
        if (lua_type(L, 1) == LUA_TSTRING)
        {
            return new T(ShadIconType::None, optionalString(L, 1), optionalString(L, 2), ShadLuaObject::get<QWidget>(L, 3));
        }
        return new T(ShadLuaObject::get<QWidget>(L, 1));
    }

    // 图片为空或编码失败时压入 nil
    void pushPngBase64(lua_State* L, const QImage& image)
    {
        QByteArray bytes;
        QBuffer buffer(&bytes);
        if (image.isNull() || !buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG"))
        {
            lua_pushnil(L);
            return;
        }
        ShadLuaStack::pushString(L, QString::fromLatin1(bytes.toBase64()));
    }

    // 设置列表模型中第 row 行的数据
    bool setRowData(ShadListView* view, int row, const QVariant& value, int role)
    {
        QAbstractItemModel* model = view->model();
        const QModelIndex index = model != nullptr ? model->index(row, 0) : QModelIndex();
        return index.isValid() && model->setData(index, value, role);
    }

    // 空状态、按键、列表项与虚拟列表
    void registerItems()
    {
        // new(icon, title, description, parent)、new(title, description, parent) 或 new(parent)
        ShadLuaClass<ShadEmpty> empty("Empty");
        empty.constructor(&newWithIconTitle<ShadEmpty>);
        empty.method("setShadIcon", &ShadEmpty::setShadIcon);
        empty.method("getShadIcon", &ShadEmpty::getShadIcon);
        empty.method("setMediaWidget", &ShadEmpty::setMediaWidget);
        empty.method("getMediaWidget", &ShadEmpty::getMediaWidget);
        empty.method("contentLayout", &ShadEmpty::contentLayout);

        ShadLuaClass<ShadKbd>("Kbd").constructor(&ShadLuaBind::newWithText<ShadKbd>);

        // new(keys, parent) 或 new(parent)
        ShadLuaClass<ShadKbdGroup> kbdGroup("KbdGroup");
        kbdGroup.constructor([](lua_State* L) -> ShadKbdGroup*
        {
            QStringList keys;
            if (ShadLuaStack::getStringList(L, 1, keys))
            {
                return new ShadKbdGroup(keys, ShadLuaObject::get<QWidget>(L, 2));
            }
            return new ShadKbdGroup(ShadLuaObject::get<QWidget>(L, 1));
        });
        kbdGroup.method("addKey", &ShadKbdGroup::addKey);
        kbdGroup.method("addText", &ShadKbdGroup::addText);
        kbdGroup.method("clear", &ShadKbdGroup::clear);
        kbdGroup.method("bindAction", &ShadKbdGroup::bindAction);
        kbdGroup.method("getBoundAction", &ShadKbdGroup::getBoundAction);
        // shad.KbdGroup.keyTexts(text)；shad.KbdGroup.shortcutText(text)
        kbdGroup.staticMethod("keyTexts", [](lua_State* L) -> int
        {
            ShadLuaStack::pushStringList(L, ShadKbdGroup::keyTexts(optionalKeySequence(L, 1)));
            return 1;
        });
        kbdGroup.staticMethod("shortcutText", [](lua_State* L) -> int
        {
            ShadLuaStack::pushString(L, ShadKbdGroup::shortcutText(optionalKeySequence(L, 1)));
            return 1;
        });

        // new(title, description, parent) 或 new(parent)
        ShadLuaClass<ShadItem> item("Item");
        item.constructor(&ShadLuaBind::newWithTitle<ShadItem>);
        item.method("setMediaIcon", &ShadItem::setMediaIcon);
        item.method("getMediaIcon", &ShadItem::getMediaIcon);
        item.method("setMediaImagePath", +[](ShadItem* self, QString path) { self->setMediaPixmap(QPixmap(path)); });
        item.method("setMediaWidget", &ShadItem::setMediaWidget);
        item.method("getMediaWidget", &ShadItem::getMediaWidget);
        item.method("addActionWidget", &ShadItem::addActionWidget);
        item.method("removeActionWidget", &ShadItem::removeActionWidget);
        item.method("setHeaderWidget", &ShadItem::setHeaderWidget);
        item.method("getHeaderWidget", &ShadItem::getHeaderWidget);
        item.method("setFooterWidget", &ShadItem::setFooterWidget);
        item.method("getFooterWidget", &ShadItem::getFooterWidget);
        item.method("setHeaderText", &ShadItem::setHeaderText);
        item.method("setFooterText", &ShadItem::setFooterText);

        ShadLuaClass<ShadItemGroup> itemGroup("ItemGroup");
        itemGroup.constructor(&ShadLuaBind::newWithParent<ShadItemGroup>);
        itemGroup.method("addItem", &ShadItemGroup::addItem);
        itemGroup.method("insertItem", &ShadItemGroup::insertItem);
        itemGroup.method("removeItem", &ShadItemGroup::removeItem);
        itemGroup.method("addSeparator", &ShadItemGroup::addSeparator);
        itemGroup.rawMethod("items", [](lua_State* L, ShadItemGroup* self) -> int
        {
            pushObjects(L, self->items());
            return 1;
        });

        // 虚拟列表，数据来自 shad.Model
        ShadLuaClass<ShadListView> listView("ListView");
        listView.constructor(&ShadLuaBind::newWithParent<ShadListView>);
        listView.method("emptyState", &ShadListView::emptyState);
        listView.method("setItemDescription", +[](ShadListView* self, int row, QString text) { return setRowData(self, row, text, ShadListView::DescriptionRole); });
        listView.method("setItemTrailingText", +[](ShadListView* self, int row, QString text) { return setRowData(self, row, text, ShadListView::TrailingTextRole); });
        listView.method("setItemIcon", +[](ShadListView* self, int row, int icon) { return setRowData(self, row, icon, ShadListView::IconRole); });
        // setItemTrailingColor(row, color)
        listView.rawMethod("setItemTrailingColor", [](lua_State* L, ShadListView* self) -> int
        {
            QVariant value;
            QColor color;
            if (lua_type(L, 3) == LUA_TNUMBER)
            {
                value = optionalInt(L, 3, 0);
            }
            else if (ShadLuaStack::getColor(L, 3, color))
            {
                value = QVariant::fromValue(color);
            }
            else if (!lua_isnoneornil(L, 3))
            {
                return ShadLuaStack::argumentError(L, 2);
            }
            lua_pushboolean(L, setRowData(self, optionalInt(L, 2, -1), value, ShadListView::TrailingColorRole) ? 1 : 0);
            return 1;
        });
        // setItemChecked(row, checked)；isItemChecked(row)
        listView.rawMethod("setItemChecked", [](lua_State* L, ShadListView* self) -> int
        {
            QStandardItemModel* model = qobject_cast<QStandardItemModel*>(self->model());
            QStandardItem* item = model != nullptr ? model->item(optionalInt(L, 2, -1), 0) : nullptr;
            if (item == nullptr)
            {
                return ShadLuaStack::error(L, QStringLiteral("行号超出范围，或列表的模型不是 shad.Model 创建的模型"));
            }
            item->setCheckable(true);
            item->setCheckState(lua_toboolean(L, 3) != 0 ? Qt::Checked : Qt::Unchecked);
            return 0;
        });
        listView.rawMethod("isItemChecked", [](lua_State* L, ShadListView* self) -> int
        {
            QAbstractItemModel* model = self->model();
            const QModelIndex index = model != nullptr ? model->index(optionalInt(L, 2, -1), 0) : QModelIndex();
            lua_pushboolean(L, index.data(Qt::CheckStateRole).toInt() == Qt::Checked ? 1 : 0);
            return 1;
        });
    }

    // 描述列表、面包屑、头像组与附件
    void registerInformation()
    {
        // new(title, parent) 或 new(parent)
        ShadLuaClass<ShadDescriptions> descriptions("Descriptions");
        descriptions.constructor(&ShadLuaBind::newWithText<ShadDescriptions>);
        descriptions.method("setExtra", &ShadDescriptions::setExtra);
        descriptions.method("getExtra", &ShadDescriptions::getExtra);
        // addItem(label, text, span) 或 addItem(label, widget, span)
        descriptions.rawMethod("addItem", [](lua_State* L, ShadDescriptions* self) -> int
        {
            QWidget* content = ShadLuaObject::get<QWidget>(L, 3);
            if (content != nullptr)
            {
                lua_pushinteger(L, self->addItem(optionalString(L, 2), content, optionalInt(L, 4, 1)));
                return 1;
            }
            lua_pushinteger(L, self->addItem(optionalString(L, 2), optionalString(L, 3), optionalInt(L, 4, 1)));
            return 1;
        });
        descriptions.method("setItemLabel", &ShadDescriptions::setItemLabel);
        descriptions.method("getItemLabel", &ShadDescriptions::getItemLabel);
        descriptions.method("setItemText", &ShadDescriptions::setItemText);
        descriptions.method("getItemText", &ShadDescriptions::getItemText);
        descriptions.method("getItemWidget", &ShadDescriptions::getItemWidget);
        descriptions.method("setItemSpan", &ShadDescriptions::setItemSpan);
        descriptions.method("getItemSpan", &ShadDescriptions::getItemSpan);
        descriptions.method("removeItem", &ShadDescriptions::removeItem);
        descriptions.method("clear", &ShadDescriptions::clear);
        descriptions.method("count", &ShadDescriptions::count);

        ShadLuaClass<ShadBreadcrumb> breadcrumb("Breadcrumb");
        breadcrumb.constructor(&ShadLuaBind::newWithParent<ShadBreadcrumb>);
        breadcrumb.method("addLink", &ShadBreadcrumb::addLink);
        breadcrumb.method("addPage", &ShadBreadcrumb::addPage);
        // addEllipsis(menu)；addDropdown(text, menu)
        breadcrumb.method("addEllipsis", &ShadBreadcrumb::addEllipsis);
        breadcrumb.method("addDropdown", &ShadBreadcrumb::addDropdown);
        breadcrumb.method("setPath", &ShadBreadcrumb::setPath);
        breadcrumb.method("clear", &ShadBreadcrumb::clear);
        breadcrumb.method("count", &ShadBreadcrumb::count);
        breadcrumb.method("setItemText", &ShadBreadcrumb::setItemText);
        breadcrumb.method("getItemText", &ShadBreadcrumb::getItemText);
        breadcrumb.method("setSeparatorIcon", &ShadBreadcrumb::setSeparatorIcon);
        breadcrumb.method("getSeparatorIcon", &ShadBreadcrumb::getSeparatorIcon);

        // addAvatar(avatar)
        ShadLuaClass<ShadAvatarGroup> avatarGroup("AvatarGroup");
        avatarGroup.constructor(&ShadLuaBind::newWithParent<ShadAvatarGroup>);
        avatarGroup.method("addAvatar", &ShadAvatarGroup::addAvatar);
        avatarGroup.method("insertAvatar", &ShadAvatarGroup::insertAvatar);
        avatarGroup.method("removeAvatar", &ShadAvatarGroup::removeAvatar);
        avatarGroup.method("count", &ShadAvatarGroup::count);
        avatarGroup.method("avatar", &ShadAvatarGroup::avatar);
        avatarGroup.method("setCountIcon", &ShadAvatarGroup::setCountIcon);
        avatarGroup.method("getCountIcon", &ShadAvatarGroup::getCountIcon);
        avatarGroup.method("clearCount", &ShadAvatarGroup::clearCount);
        avatarGroup.method("hasCount", &ShadAvatarGroup::hasCount);

        // new(icon, title, description, parent)、new(title, description, parent) 或 new(parent)
        ShadLuaClass<ShadAttachment> attachment("Attachment");
        attachment.constructor(&newWithIconTitle<ShadAttachment>);
        attachment.method("setShadIcon", &ShadAttachment::setShadIcon);
        attachment.method("getShadIcon", &ShadAttachment::getShadIcon);
        attachment.method("setImagePath", +[](ShadAttachment* self, QString path) { self->setImage(QPixmap(path)); });
        // addAction(icon, toolTip)
        attachment.method("addAction", &ShadAttachment::addAction);
        attachment.rawMethod("getActions", [](lua_State* L, ShadAttachment* self) -> int
        {
            pushObjects(L, self->getActions());
            return 1;
        });
        attachment.method("clearActions", &ShadAttachment::clearActions);
        attachment.method("setFile", &ShadAttachment::setFile);

        ShadLuaClass<ShadAttachmentGroup> attachmentGroup("AttachmentGroup");
        attachmentGroup.constructor(&ShadLuaBind::newWithParent<ShadAttachmentGroup>);
        attachmentGroup.method("addAttachment", &ShadAttachmentGroup::addAttachment);
        attachmentGroup.method("insertAttachment", &ShadAttachmentGroup::insertAttachment);
        attachmentGroup.method("removeAttachment", &ShadAttachmentGroup::removeAttachment);
        attachmentGroup.method("clear", &ShadAttachmentGroup::clear);
        attachmentGroup.method("count", &ShadAttachmentGroup::count);
        attachmentGroup.method("attachmentAt", &ShadAttachmentGroup::attachmentAt);
        attachmentGroup.method("scrollToAttachment", &ShadAttachmentGroup::scrollToAttachment);

        ShadLuaClass<ShadTimelineItem> timelineItem;
        timelineItem.method("setIcon", &ShadTimelineItem::setIcon);
        timelineItem.method("getIcon", &ShadTimelineItem::getIcon);
        timelineItem.method("contentLayout", &ShadTimelineItem::contentLayout);

        ShadLuaClass<ShadTimeline> timeline("Timeline");
        timeline.constructor(&ShadLuaBind::newWithParent<ShadTimeline>);
        timeline.rawMethod("addItem", [](lua_State* L, ShadTimeline* self) -> int
        {
            ShadLuaObject::push(L, self->addItem(optionalString(L, 2), optionalString(L, 3), optionalString(L, 4)));
            return 1;
        });
        timeline.method("itemAt", &ShadTimeline::itemAt);
        timeline.method("count", &ShadTimeline::count);
        timeline.method("clear", &ShadTimeline::clear);
        timeline.method("pendingItem", &ShadTimeline::pendingItem);
    }

    // Markdown 视图、图片预览与图片裁剪
    void registerMedia()
    {
        ShadLuaClass<ShadMarkdownView> markdownView("MarkdownView");
        markdownView.constructor(&ShadLuaBind::newWithParent<ShadMarkdownView>);
        markdownView.method("appendMarkdown", &ShadMarkdownView::appendMarkdown);
        markdownView.method("clear", &ShadMarkdownView::clear);
        markdownView.method("setBaseUrl", +[](ShadMarkdownView* self, QString url) { self->setBaseUrl(QUrl(url)); });
        markdownView.method("getBaseUrl", +[](ShadMarkdownView* self) { return self->getBaseUrl().toString(); });

        // new(parent)，parent 决定覆盖的窗口
        ShadLuaClass<ShadImageViewer> imageViewer("ImageViewer");
        imageViewer.constructor(&ShadLuaBind::newWithParent<ShadImageViewer>);
        imageViewer.method("setImagePaths", &ShadImageViewer::setImagePaths);
        imageViewer.method("getImagePaths", &ShadImageViewer::getImagePaths);
        imageViewer.method("getImageCount", &ShadImageViewer::getImageCount);
        imageViewer.rawMethod("open", [](lua_State* L, ShadImageViewer* self) -> int
        {
            self->open(optionalInt(L, 2, 0));
            return 0;
        });
        imageViewer.method("isOpen", &ShadImageViewer::isOpen);
        // shad.ImageViewer.preview(parent, paths, index)，关闭后自动释放
        imageViewer.staticMethod("preview", [](lua_State* L) -> int
        {
            QStringList paths;
            if (!ShadLuaStack::getStringList(L, 2, paths))
            {
                return ShadLuaStack::argumentError(L, 2);
            }
            ShadImageViewer::preview(ShadLuaObject::get<QWidget>(L, 1), paths, optionalInt(L, 3, 0));
            return 0;
        });

        ShadLuaClass<ShadImageCropper> imageCropper("ImageCropper");
        imageCropper.constructor(&ShadLuaBind::newWithParent<ShadImageCropper>);
        imageCropper.method("setImagePath", +[](ShadImageCropper* self, QString path)
        {
            const QImage image(path);
            if (image.isNull())
            {
                return false;
            }
            self->setImage(image);
            return true;
        });
        imageCropper.method("rotateLeft", &ShadImageCropper::rotateLeft);
        imageCropper.method("rotateRight", &ShadImageCropper::rotateRight);
        imageCropper.method("flipHorizontal", &ShadImageCropper::flipHorizontal);
        imageCropper.method("flipVertical", &ShadImageCropper::flipVertical);
        imageCropper.method("reset", &ShadImageCropper::reset);
        // 裁剪区域用 {x, y, 宽, 高} 设置
        imageCropper.rawMethod("getCropRect", [](lua_State* L, ShadImageCropper* self) -> int
        {
            ShadLuaStack::pushVariant(L, QVariant(self->getCropRect()));
            return 1;
        });
        imageCropper.rawMethod("setCropRect", [](lua_State* L, ShadImageCropper* self) -> int
        {
            const QVariant rect = ShadLuaStack::getVariant(L, 2, QMetaType::fromType<QRect>());
            if (rect.typeId() != QMetaType::QRect)
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            self->setCropRect(rect.toRect());
            return 0;
        });
        // croppedPngBase64()
        imageCropper.rawMethod("croppedPngBase64", [](lua_State* L, ShadImageCropper* self) -> int
        {
            pushPngBase64(L, self->croppedImage());
            return 1;
        });
    }
} // namespace

namespace ShadLuaBindings
{
    void registerDisplayComponents()
    {
        registerItems();
        registerInformation();
        registerMedia();
    }
} // namespace ShadLuaBindings

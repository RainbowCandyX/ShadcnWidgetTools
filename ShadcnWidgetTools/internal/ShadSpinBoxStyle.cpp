#include "ShadSpinBoxStyle.h"

#include <QAction>
#include <QClipboard>
#include <QColor>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QKeySequence>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QResizeEvent>
#include <QString>
#include <QStyleOptionSpinBox>
#include <QWidget>
#include <QtGlobal>

#include "ShadDropdownMenu.h"
#include "ShadIcon.h"
#include "ShadKbd.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // 可视高度 h-9
    constexpr int kBodyHeight = 36;
    constexpr int kBorderWidth = 1;
    // 文字的水平内边距 px-3，与按钮之间的间距
    constexpr int kPaddingX = 12;
    constexpr int kTextGap = 4;
    // Inline / Side 布局的按钮：28px，距外框 4px，Inline 两个按钮之间 2px
    constexpr int kButtonSize = 28;
    constexpr int kButtonInset = 4;
    constexpr int kButtonGap = 2;
    constexpr int kButtonIconSize = 16;
    // Compact 布局的按钮列宽度与图标
    constexpr int kCompactColumnWidth = 24;
    constexpr int kCompactIconSize = 14;
    // 输入区为光标预留的宽度
    constexpr int kCursorWidth = 2;
} // namespace

ShadSpinBoxStyle::ShadSpinBoxStyle(QAbstractSpinBox* spinBox)
    : QProxyStyle(), _spinBox(spinBox)
{
    setParent(spinBox);
    // 按钮的悬停态依赖 HoverMove 事件
    spinBox->setAttribute(Qt::WA_Hover, true);
#ifdef Q_OS_MACOS
    spinBox->setAttribute(Qt::WA_MacShowFocusRect, false);
#endif
    // 滚轮只在获得焦点后改变数值，不随滚轮获得焦点
    spinBox->setFocusPolicy(Qt::StrongFocus);
    ShadStyleHelper::applyFont(spinBox, 14, QFont::Normal);
    _lineEdit = spinBox->findChild<QLineEdit*>(QString(), Qt::FindDirectChildrenOnly);
    if (_lineEdit != nullptr)
    {
        _lineEdit->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }
    applyThemeColors();
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]()
    {
        applyThemeColors();
        if (!_spinBox.isNull())
        {
            _spinBox->update();
        }
    });
}

ShadSpinBoxStyle::~ShadSpinBoxStyle()
{
}

void ShadSpinBoxStyle::setButtonLayout(ShadSpinBoxType::ButtonLayout layout)
{
    if (_layout == layout)
    {
        return;
    }
    _layout = layout;
    // 两侧放按钮时文字居中；QLineEdit 的 AlignLeft 在 RTL 下自动表示右侧
    const bool isCentered = layout == ShadSpinBoxType::Side || layout == ShadSpinBoxType::PlusMinusSide;
    if (_lineEdit != nullptr)
    {
        _lineEdit->setAlignment(isCentered ? Qt::AlignCenter : (Qt::AlignLeft | Qt::AlignVCenter));
    }
    relayout();
}

ShadSpinBoxType::ButtonLayout ShadSpinBoxStyle::getButtonLayout() const
{
    return _layout;
}

void ShadSpinBoxStyle::setInvalid(bool invalid)
{
    _isInvalid = invalid;
    if (!_spinBox.isNull())
    {
        _spinBox->update();
    }
}

bool ShadSpinBoxStyle::isInvalid() const
{
    return _isInvalid;
}

QSize ShadSpinBoxStyle::sizeHintForTexts(const QStringList& texts) const
{
    int textWidth = 0;
    if (!_spinBox.isNull())
    {
        for (const QString& text : texts)
        {
            textWidth = qMax(textWidth, ShadStyleHelper::textWidth(_spinBox->font(), text));
        }
    }
    textWidth += kCursorWidth;
    int chromeWidth = 0;
    switch (_layout)
    {
        case ShadSpinBoxType::Compact:
            {
                chromeWidth = kBorderWidth + kPaddingX + kTextGap + kCompactColumnWidth + kBorderWidth;
                break;
            }
        case ShadSpinBoxType::Side:
        case ShadSpinBoxType::PlusMinusSide:
            {
                chromeWidth = 2 * (kButtonInset + kButtonSize + kTextGap);
                break;
            }
        case ShadSpinBoxType::Inline:
            {
                chromeWidth = kBorderWidth + kPaddingX + kTextGap + 2 * kButtonSize + kButtonGap + kButtonInset;
                break;
            }
    }
    const int ring = 2 * ShadStyleHelper::kFocusRingWidth;
    return QSize(textWidth + chromeWidth + ring, kBodyHeight + ring);
}

void ShadSpinBoxStyle::showContextMenu(QAbstractSpinBox* spinBox, QLineEdit* lineEdit, QAbstractSpinBox::StepEnabled stepEnabled, const QPoint& globalPos)
{
    if (spinBox == nullptr || lineEdit == nullptr)
    {
        return;
    }
    // 每次弹出时新建，关闭后删除；菜单项触发在关闭之后、删除之前
    ShadDropdownMenu* menu = new ShadDropdownMenu(spinBox);
    QObject::connect(menu, &ShadDropdownMenu::aboutToHide, menu, &QObject::deleteLater);
    const bool isEditable = !lineEdit->isReadOnly();
    const bool hasSelection = lineEdit->hasSelectedText();
    if (isEditable)
    {
        QAction* undoAction = menu->addItem(QCoreApplication::translate("ShadSpinBox", "撤销"), ShadIconType::Undo2, ShadKbdGroup::shortcutText(QKeySequence(QKeySequence::Undo)));
        undoAction->setEnabled(lineEdit->isUndoAvailable());
        QObject::connect(undoAction, &QAction::triggered, lineEdit, &QLineEdit::undo);
        QAction* redoAction = menu->addItem(QCoreApplication::translate("ShadSpinBox", "恢复"), ShadIconType::Redo2, ShadKbdGroup::shortcutText(QKeySequence(QKeySequence::Redo)));
        redoAction->setEnabled(lineEdit->isRedoAvailable());
        QObject::connect(redoAction, &QAction::triggered, lineEdit, &QLineEdit::redo);
        menu->addSeparator();
        QAction* cutAction = menu->addItem(QCoreApplication::translate("ShadSpinBox", "剪切"), ShadIconType::Scissors, ShadKbdGroup::shortcutText(QKeySequence(QKeySequence::Cut)));
        cutAction->setEnabled(hasSelection);
        QObject::connect(cutAction, &QAction::triggered, lineEdit, &QLineEdit::cut);
    }
    QAction* copyAction = menu->addItem(QCoreApplication::translate("ShadSpinBox", "复制"), ShadIconType::Copy, ShadKbdGroup::shortcutText(QKeySequence(QKeySequence::Copy)));
    copyAction->setEnabled(hasSelection);
    QObject::connect(copyAction, &QAction::triggered, lineEdit, &QLineEdit::copy);
    if (isEditable)
    {
        QAction* pasteAction = menu->addItem(QCoreApplication::translate("ShadSpinBox", "粘贴"), ShadIconType::ClipboardPaste, ShadKbdGroup::shortcutText(QKeySequence(QKeySequence::Paste)));
        const QClipboard* clipboard = QGuiApplication::clipboard();
        pasteAction->setEnabled(clipboard != nullptr && !clipboard->text().isEmpty());
        QObject::connect(pasteAction, &QAction::triggered, lineEdit, &QLineEdit::paste);
        QAction* deleteAction = menu->addItem(QCoreApplication::translate("ShadSpinBox", "删除"), ShadIconType::Delete);
        deleteAction->setEnabled(hasSelection);
        QObject::connect(deleteAction, &QAction::triggered, lineEdit, &QLineEdit::del);
    }
    menu->addSeparator();
    QAction* selectAllAction = menu->addItem(QCoreApplication::translate("ShadSpinBox", "全选"), ShadIconType::TextSelect, ShadKbdGroup::shortcutText(QKeySequence(QKeySequence::SelectAll)));
    selectAllAction->setEnabled(!lineEdit->text().isEmpty() && lineEdit->selectedText() != lineEdit->text());
    QObject::connect(selectAllAction, &QAction::triggered, spinBox, &QAbstractSpinBox::selectAll);
    menu->addSeparator();
    QAction* stepUpAction = menu->addItem(QCoreApplication::translate("ShadSpinBox", "增加"), ShadIconType::Plus);
    stepUpAction->setEnabled(stepEnabled.testFlag(QAbstractSpinBox::StepUpEnabled));
    QObject::connect(stepUpAction, &QAction::triggered, spinBox, &QAbstractSpinBox::stepUp);
    QAction* stepDownAction = menu->addItem(QCoreApplication::translate("ShadSpinBox", "减少"), ShadIconType::Minus);
    stepDownAction->setEnabled(stepEnabled.testFlag(QAbstractSpinBox::StepDownEnabled));
    QObject::connect(stepDownAction, &QAction::triggered, spinBox, &QAbstractSpinBox::stepDown);
    menu->popupAt(globalPos);
}

void ShadSpinBoxStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget) const
{
    const QStyleOptionSpinBox* spinOption = qstyleoption_cast<const QStyleOptionSpinBox*>(option);
    if (control != CC_SpinBox || spinOption == nullptr)
    {
        QProxyStyle::drawComplexControl(control, option, painter, widget);
        return;
    }
    using namespace ShadStyleHelper;
    const bool isEnabled = option->state.testFlag(State_Enabled);
    const bool isFocused = option->state.testFlag(State_HasFocus);
    const QRectF bodyRect = insetRect(option->rect, kFocusRingWidth);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter->setOpacity(isEnabled ? 1.0 : kDisabledOpacity);

    // 外框：与 ShadInput 相同，对应 border-input shadow-xs dark:bg-input/30，focus-visible:border-ring ring-ring/50 ring-[3px]
    drawShadowXs(painter, bodyRect, radius);
    if (shadTheme->isDark())
    {
        fillRoundedRect(painter, bodyRect, radius, withAlpha(shadTheme->getColor(ShadThemeType::Input), 0.3));
    }
    QColor borderColor = shadTheme->getColor(isFocused ? ShadThemeType::Ring : ShadThemeType::Input);
    if (_isInvalid)
    {
        borderColor = shadTheme->getColor(ShadThemeType::Destructive);
    }

    // Compact 布局的按钮列：起始侧与两个按钮之间各有一条 border-input 分隔线，悬停背景裁剪在外框内
    if (_layout == ShadSpinBoxType::Compact)
    {
        const QRect upRect = subControlRect(control, option, SC_SpinBoxUp, widget);
        const QRect downRect = subControlRect(control, option, SC_SpinBoxDown, widget);
        painter->save();
        painter->setClipPath(roundedRectPath(bodyRect, radius));
        drawButton(painter, option, SC_SpinBoxUp, spinOption->stepEnabled.testFlag(QAbstractSpinBox::StepUpEnabled));
        drawButton(painter, option, SC_SpinBoxDown, spinOption->stepEnabled.testFlag(QAbstractSpinBox::StepDownEnabled));
        painter->restore();
        const QColor lineColor = shadTheme->getColor(ShadThemeType::Input);
        const bool isRightToLeft = option->direction == Qt::RightToLeft;
        const qreal lineX = isRightToLeft ? upRect.right() + 1.5 : upRect.left() - 0.5;
        painter->setPen(QPen(lineColor, 1.0));
        painter->drawLine(QPointF(lineX, upRect.top()), QPointF(lineX, downRect.bottom() + 1));
        const qreal lineY = downRect.top() - 0.5;
        painter->drawLine(QPointF(upRect.left(), lineY), QPointF(upRect.right() + 1, lineY));
    }
    else
    {
        drawButton(painter, option, SC_SpinBoxUp, spinOption->stepEnabled.testFlag(QAbstractSpinBox::StepUpEnabled));
        drawButton(painter, option, SC_SpinBoxDown, spinOption->stepEnabled.testFlag(QAbstractSpinBox::StepDownEnabled));
    }

    strokeRoundedRect(painter, bodyRect, radius, borderColor);
    // 输入框获得焦点即显示焦点环（与 ShadInput 一致）；错误态常驻 destructive 焦点环
    if (isFocused || _isInvalid)
    {
        drawFocusRing(painter, bodyRect, radius, focusRingColor(_isInvalid));
    }
    painter->restore();
}

QRect ShadSpinBoxStyle::subControlRect(ComplexControl control, const QStyleOptionComplex* option, SubControl subControl, const QWidget* widget) const
{
    if (control != CC_SpinBox)
    {
        return QProxyStyle::subControlRect(control, option, subControl, widget);
    }
    switch (subControl)
    {
        case SC_SpinBoxFrame:
            {
                return option->rect;
            }
        case SC_SpinBoxUp:
        case SC_SpinBoxDown:
        case SC_SpinBoxEditField:
            {
                // 按 LTR 计算，RTL 时在组件内水平镜像
                return ShadStyleHelper::visualRect(option->direction, option->rect, logicalSubControlRect(option->rect, subControl));
            }
        default:
            {
                break;
            }
    }
    return QProxyStyle::subControlRect(control, option, subControl, widget);
}

void ShadSpinBoxStyle::applyThemeColors()
{
    if (_lineEdit == nullptr)
    {
        return;
    }
    using ShadStyleHelper::withAlpha;
    const QColor foreground = shadTheme->getColor(ShadThemeType::Foreground);
    const QColor selection = shadTheme->getColor(ShadThemeType::Primary);
    const QColor selectionText = shadTheme->getColor(ShadThemeType::PrimaryForeground);
    // 背景与边框由数字输入框自绘，样式表只清空原生外观并设置文字颜色
    const QString styleSheet = QStringLiteral("QLineEdit{background-color:transparent;border:none;padding:0px;color:%1;selection-background-color:%2;selection-color:%3;}QLineEdit:disabled{color:%4;}").arg(foreground.name(QColor::HexArgb), selection.name(QColor::HexArgb), selectionText.name(QColor::HexArgb), withAlpha(foreground, ShadStyleHelper::kDisabledOpacity).name(QColor::HexArgb));
    _lineEdit->setStyleSheet(styleSheet);
}

void ShadSpinBoxStyle::relayout()
{
    if (_spinBox.isNull())
    {
        return;
    }
    // QAbstractSpinBox 在 resizeEvent 中按 SC_SpinBoxEditField 放置内部输入框
    QResizeEvent event(_spinBox->size(), _spinBox->size());
    QCoreApplication::sendEvent(_spinBox.data(), &event);
    _spinBox->updateGeometry();
    _spinBox->update();
}

QRect ShadSpinBoxStyle::logicalSubControlRect(const QRect& rect, SubControl subControl) const
{
    const int ring = ShadStyleHelper::kFocusRingWidth;
    const QRect body = rect.adjusted(ring, ring, -ring, -ring);
    const int bodyLeft = body.left();
    const int bodyRight = body.left() + body.width();
    const int innerTop = body.top() + kBorderWidth;
    const int innerHeight = body.height() - 2 * kBorderWidth;
    const int buttonTop = body.top() + (body.height() - kButtonSize) / 2;
    QRect upRect;
    QRect downRect;
    int editLeft = 0;
    int editRight = 0;
    switch (_layout)
    {
        case ShadSpinBoxType::Compact:
            {
                const int columnLeft = bodyRight - kBorderWidth - kCompactColumnWidth;
                const int upHeight = innerHeight / 2;
                upRect = QRect(columnLeft, innerTop, kCompactColumnWidth, upHeight);
                downRect = QRect(columnLeft, innerTop + upHeight, kCompactColumnWidth, innerHeight - upHeight);
                editLeft = bodyLeft + kBorderWidth + kPaddingX;
                editRight = columnLeft - kTextGap;
                break;
            }
        case ShadSpinBoxType::Side:
        case ShadSpinBoxType::PlusMinusSide:
            {
                downRect = QRect(bodyLeft + kButtonInset, buttonTop, kButtonSize, kButtonSize);
                upRect = QRect(bodyRight - kButtonInset - kButtonSize, buttonTop, kButtonSize, kButtonSize);
                editLeft = downRect.left() + kButtonSize + kTextGap;
                editRight = upRect.left() - kTextGap;
                break;
            }
        case ShadSpinBoxType::Inline:
            {
                upRect = QRect(bodyRight - kButtonInset - kButtonSize, buttonTop, kButtonSize, kButtonSize);
                downRect = QRect(upRect.left() - kButtonGap - kButtonSize, buttonTop, kButtonSize, kButtonSize);
                editLeft = bodyLeft + kBorderWidth + kPaddingX;
                editRight = downRect.left() - kTextGap;
                break;
            }
    }
    switch (subControl)
    {
        case SC_SpinBoxUp:
            return upRect;
        case SC_SpinBoxDown:
            return downRect;
        default:
            break;
    }
    return QRect(editLeft, innerTop, qMax(0, editRight - editLeft), innerHeight);
}

void ShadSpinBoxStyle::drawButton(QPainter* painter, const QStyleOptionComplex* option, SubControl subControl, bool isStepEnabled) const
{
    using namespace ShadStyleHelper;
    const QRect buttonRect = subControlRect(CC_SpinBox, option, subControl, nullptr);
    const bool isEnabled = isStepEnabled && option->state.testFlag(State_Enabled);
    const bool isActive = isEnabled && option->activeSubControls.testFlag(subControl);
    const bool isPressed = isActive && option->state.testFlag(State_Sunken);
    const bool isHovered = isActive && option->state.testFlag(State_MouseOver);
    // 按钮为 ghost 风格：hover:bg-accent（暗色 accent/50），按下时再叠加一层前景色
    if (isHovered || isPressed)
    {
        const QColor accent = shadTheme->getColor(ShadThemeType::Accent);
        QColor background = shadTheme->isDark() ? withAlpha(accent, 0.5) : accent;
        if (isPressed)
        {
            background = mix(background, shadTheme->getColor(ShadThemeType::Foreground), 0.08);
        }
        // Compact 的按钮贴住外框，由调用方裁剪到外框内，不需要圆角
        const qreal radius = _layout == ShadSpinBoxType::Compact ? 0.0 : qMax(0, shadTheme->getBaseRadius() - 4);
        fillRoundedRect(painter, QRectF(buttonRect), radius, background);
    }
    const bool isUp = subControl == SC_SpinBoxUp;
    ShadIconType::IconName icon = ShadIconType::None;
    switch (_layout)
    {
        case ShadSpinBoxType::Inline:
        case ShadSpinBoxType::PlusMinusSide:
            icon = isUp ? ShadIconType::Plus : ShadIconType::Minus;
            break;
        case ShadSpinBoxType::Compact:
        case ShadSpinBoxType::Side:
            icon = isUp ? ShadIconType::ChevronUp : ShadIconType::ChevronDown;
            break;
    }
    // 不能继续增减时按钮为 50% 不透明度（对应 disabled:opacity-50）
    QColor iconColor = shadTheme->getColor(isHovered || isPressed ? ShadThemeType::Foreground : ShadThemeType::MutedForeground);
    if (!isStepEnabled)
    {
        iconColor = withAlpha(iconColor, kDisabledOpacity);
    }
    ShadIcon::paint(painter, QRectF(buttonRect), icon, iconColor, _layout == ShadSpinBoxType::Compact ? kCompactIconSize : kButtonIconSize);
}

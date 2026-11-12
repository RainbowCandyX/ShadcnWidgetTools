#ifndef SHADDEF_H
#define SHADDEF_H

#include <QObject>

#include "ShadProperty.h"

namespace ShadThemeType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum ThemeMode
    {
        Light = 0,
        Dark,
    };

    Q_ENUM_NS(ThemeMode)

    // 主题颜色 token，与 shadcn/ui 的 CSS 变量一一对应
    enum ThemeColor
    {
        Background = 0,
        Foreground,
        Card,
        CardForeground,
        Popover,
        PopoverForeground,
        Primary,
        PrimaryForeground,
        Secondary,
        SecondaryForeground,
        Muted,
        MutedForeground,
        Accent,
        AccentForeground,
        Destructive,
        DestructiveForeground,
        Border,
        Input,
        Ring,
        Sidebar,
        SidebarForeground,
        SidebarPrimary,
        SidebarPrimaryForeground,
        SidebarAccent,
        SidebarAccentForeground,
        SidebarBorder,
        SidebarRing,
        // 图表配色不随基色变化
        Chart1,
        Chart2,
        Chart3,
        Chart4,
        Chart5,
        ColorCount,
    };

    Q_ENUM_NS(ThemeColor)

    enum Radius
    {
        Small = 0,
        Medium,
        Large,
        ExtraLarge,
    };

    Q_ENUM_NS(Radius)

    enum BaseColor
    {
        Neutral = 0,
        Zinc,
        Slate,
        Stone,
        Gray,
    };

    Q_ENUM_NS(BaseColor)

    enum PageTransitionStyle
    {
        Fade = 0,
        Slide,
    };

    Q_ENUM_NS(PageTransitionStyle)
} // namespace ShadThemeType

namespace ShadButtonType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Variant
    {
        Default = 0,
        Destructive,
        Outline,
        Secondary,
        Ghost,
        Link,
    };

    Q_ENUM_NS(Variant)

    enum Size
    {
        Medium = 0,
        Small,
        Large,
        Icon,
        IconSmall,
        IconLarge,
        IconExtraSmall,
        ExtraSmall,
    };

    Q_ENUM_NS(Size)
} // namespace ShadButtonType

namespace ShadBadgeType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Variant
    {
        Default = 0,
        Secondary,
        Destructive,
        Outline,
    };

    Q_ENUM_NS(Variant)
} // namespace ShadBadgeType

namespace ShadToggleType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Variant
    {
        Default = 0,
        Outline,
    };

    Q_ENUM_NS(Variant)

    enum Size
    {
        Medium = 0,
        Small,
        Large,
    };

    Q_ENUM_NS(Size)
} // namespace ShadToggleType

namespace ShadToggleGroupType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum SelectionMode
    {
        Single = 0,
        Multiple,
    };

    Q_ENUM_NS(SelectionMode)
} // namespace ShadToggleGroupType

namespace ShadInputType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum TrailingAction
    {
        NoAction = 0,
        ClearAction,
        PasswordToggleAction,
    };

    Q_ENUM_NS(TrailingAction)
} // namespace ShadInputType

namespace ShadToastType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Type
    {
        Default = 0,
        Success,
        Info,
        Warning,
        Error,
        Loading,
    };

    Q_ENUM_NS(Type)

    // Left / Right 为逻辑方向，RTL 下左右互换
    enum Position
    {
        TopLeft = 0,
        TopCenter,
        TopRight,
        BottomLeft,
        BottomCenter,
        BottomRight,
    };

    Q_ENUM_NS(Position)
} // namespace ShadToastType

namespace ShadNotificationType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Type
    {
        Info = 0,
        Success,
        Warning,
        Error,
    };

    Q_ENUM_NS(Type)

    enum Position
    {
        Top = 0,
        Bottom,
        Left,
        Right,
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight,
    };

    Q_ENUM_NS(Position)
}

namespace ShadRadialProgressType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Variant
    {
        Circle = 0,
        Gauge,
    };

    Q_ENUM_NS(Variant)

    enum Status
    {
        Default = 0,
        Success,
        Warning,
        Destructive,
    };

    Q_ENUM_NS(Status)
}

namespace ShadStatisticType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Trend
    {
        None = 0,
        Up,
        Down,
    };

    Q_ENUM_NS(Trend)
}

namespace ShadSegmentedType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Size
    {
        Small = 0,
        Default,
        Large,
    };

    Q_ENUM_NS(Size)
}

namespace ShadImageCropperType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Shape
    {
        Rectangle = 0,
        Circle,
    };

    Q_ENUM_NS(Shape)
}

namespace ShadMarkerType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Variant
    {
        Default = 0,
        Border,
        Separator,
    };

    Q_ENUM_NS(Variant)
}

namespace ShadAlertType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Variant
    {
        Default = 0,
        Destructive,
    };

    Q_ENUM_NS(Variant)
} // namespace ShadAlertType

namespace ShadPlacementType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    // 空间不足时翻到对侧，RTL 下 Left / Right 互换
    enum Side
    {
        Top = 0,
        Bottom,
        Left,
        Right,
    };

    Q_ENUM_NS(Side)

    // Start / End 为逻辑方向，RTL 下起始侧为右侧
    enum Align
    {
        Start = 0,
        Center,
        End,
    };

    Q_ENUM_NS(Align)
} // namespace ShadPlacementType

namespace ShadInputGroupType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Control
    {
        LineEdit = 0,
        PlainText,
    };

    Q_ENUM_NS(Control)

    enum AddonPosition
    {
        InlineStart = 0,
        InlineEnd,
        BlockStart,
        BlockEnd,
    };

    Q_ENUM_NS(AddonPosition)
} // namespace ShadInputGroupType

namespace ShadAttachmentType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum State
    {
        Idle = 0,
        Uploading,
        Processing,
        Error,
        Done,
    };

    Q_ENUM_NS(State)

    enum Size
    {
        Medium = 0,
        Small,
        ExtraSmall,
    };

    Q_ENUM_NS(Size)
} // namespace ShadAttachmentType

namespace ShadCodeBlockType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Variant
    {
        Default = 0,
        Destructive,
    };

    Q_ENUM_NS(Variant)

    enum Size
    {
        Medium = 0,
        Small,
    };

    Q_ENUM_NS(Size)

    enum LineVariant
    {
        NormalLine = 0,
        AddedLine,
        RemovedLine,
        HighlightedLine,
    };

    Q_ENUM_NS(LineVariant)
} // namespace ShadCodeBlockType

namespace ShadTraceType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    // 状态仅靠颜色与动画表示，标题应同时写明状态
    enum Status
    {
        Done = 0,
        Running,
        Error,
        Pending,
    };

    Q_ENUM_NS(Status)
} // namespace ShadTraceType

namespace ShadTabsType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Variant
    {
        Default = 0,
        Line,
    };

    Q_ENUM_NS(Variant)
} // namespace ShadTabsType

namespace ShadTabViewType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum TabFlag
    {
        NoTabFlag      = 0x0,
        TabNotClosable = 0x1,
        TabNotMovable  = 0x2,
        TabDisabled    = 0x4,
        AllTabFlags    = TabNotClosable | TabNotMovable | TabDisabled,
    };

    Q_DECLARE_FLAGS(TabFlags, TabFlag)
    Q_FLAG_NS (TabFlags)
} // namespace ShadTabViewType

Q_DECLARE_OPERATORS_FOR_FLAGS (ShadTabViewType::TabFlags)

namespace ShadDockType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum DockArea
    {
        LeftArea = 0,
        RightArea,
        TopArea,
        BottomArea,
        CenterArea,
    };

    Q_ENUM_NS(DockArea)

    enum PanelFlag
    {
        NoPanelFlag       = 0x0,
        PanelNotClosable  = 0x1,
        PanelNotMovable   = 0x2,
        PanelNotFloatable = 0x4,
        AllPanelFlags     = PanelNotClosable | PanelNotMovable | PanelNotFloatable,
    };

    Q_DECLARE_FLAGS(PanelFlags, PanelFlag)
    Q_FLAG_NS (PanelFlags)
} // namespace ShadDockType

Q_DECLARE_OPERATORS_FOR_FLAGS (ShadDockType::PanelFlags)

namespace ShadDrawerType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    // 物理方向，不随 RTL 镜像
    enum SwipeDirection
    {
        Up = 0,
        Right,
        Down,
        Left,
    };

    Q_ENUM_NS(SwipeDirection)
} // namespace ShadDrawerType

namespace ShadQuestionnaireType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Shortcuts
    {
        NoShortcuts = 0,
        Letters,
        Numbers,
    };

    Q_ENUM_NS(Shortcuts)

    enum ItemStatus
    {
        Unanswered = 0,
        Answered,
        Skipped,
    };

    Q_ENUM_NS(ItemStatus)
} // namespace ShadQuestionnaireType

namespace ShadAccordionType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Variant
    {
        Default = 0,
        Bordered,
    };

    Q_ENUM_NS(Variant)
} // namespace ShadAccordionType

namespace ShadComboBoxType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Size
    {
        Medium = 0,
        Small,
    };

    Q_ENUM_NS(Size)

    enum SelectionMode
    {
        SingleSelection = 0,
        MultiSelection,
    };

    Q_ENUM_NS(SelectionMode)
} // namespace ShadComboBoxType

namespace ShadWindowType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    // StayOnTopButton 默认隐藏，AllButtons 不包含它
    enum ButtonFlag
    {
        NoButton        = 0x0,
        MinimizeButton  = 0x1,
        MaximizeButton  = 0x2,
        CloseButton     = 0x4,
        StayOnTopButton = 0x8,
        AllButtons      = MinimizeButton | MaximizeButton | CloseButton,
    };

    Q_DECLARE_FLAGS(ButtonFlags, ButtonFlag)
    Q_FLAG_NS (ButtonFlags)

    enum TitleBarArea
    {
        LeftArea = 0,
        RightArea,
    };

    Q_ENUM_NS(TitleBarArea)

    enum ButtonPosition
    {
        ButtonsOnRight = 0,
        ButtonsOnLeft,
    };

    Q_ENUM_NS(ButtonPosition)

    // 平台支持情况见 ShadTheme::isWindowEffectSupported
    enum Effect
    {
        NoEffect = 0,
        Wallpaper,
        Blur,
        Acrylic,
        Mica,
        MicaAlt,
    };

    Q_ENUM_NS(Effect)
} // namespace ShadWindowType

Q_DECLARE_OPERATORS_FOR_FLAGS (ShadWindowType::ButtonFlags)

namespace ShadQrCodeType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    // 纠错等级越高，同样内容占用的版本越大
    enum ErrorCorrection
    {
        Low = 0,
        Medium,
        Quartile,
        High,
    };

    Q_ENUM_NS(ErrorCorrection)
} // namespace ShadQrCodeType

namespace ShadCalendarType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum SelectionMode
    {
        Single = 0,
        Multiple,
        Range,
    };

    Q_ENUM_NS(SelectionMode)

    enum CaptionLayout
    {
        Label = 0,
        Dropdown,
    };

    Q_ENUM_NS(CaptionLayout)
} // namespace ShadCalendarType

namespace ShadDatePickerType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Mode
    {
        SingleDate = 0,
        DateRange,
    };

    Q_ENUM_NS(Mode)
} // namespace ShadDatePickerType

namespace ShadSidebarType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    // Start / End 为逻辑方向，RTL 下左右互换
    enum Side
    {
        Start = 0,
        End,
    };

    Q_ENUM_NS(Side)

    enum Variant
    {
        Sidebar = 0,
        Floating,
        Inset,
    };

    Q_ENUM_NS(Variant)

    enum Collapsible
    {
        Offcanvas = 0,
        Icon,
        NotCollapsible,
    };

    Q_ENUM_NS(Collapsible)
} // namespace ShadSidebarType

namespace ShadSidebarMenuButtonType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Size
    {
        Small = 0,
        Medium,
        Large,
    };

    Q_ENUM_NS(Size)

    enum Variant
    {
        Default = 0,
        Outline,
    };

    Q_ENUM_NS(Variant)
} // namespace ShadSidebarMenuButtonType

namespace ShadCarouselType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    // Start / End 为逻辑方向，RTL 下起始侧为右侧
    enum Align
    {
        Start = 0,
        Center,
        End,
    };

    Q_ENUM_NS(Align)
} // namespace ShadCarouselType

namespace ShadAvatarType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Size
    {
        Small = 0,
        Medium,
        Large,
    };

    Q_ENUM_NS(Size)

    enum Shape
    {
        Circle = 0,
        Rounded,
    };

    Q_ENUM_NS(Shape)
} // namespace ShadAvatarType

namespace ShadSpinBoxType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    // 按钮位置按逻辑方向，RTL 下左右互换
    enum ButtonLayout
    {
        Inline = 0,
        Compact,
        Side,
        PlusMinusSide,
    };

    Q_ENUM_NS(ButtonLayout)
} // namespace ShadSpinBoxType

namespace ShadFieldType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Orientation
    {
        Vertical = 0,
        Horizontal,
        Responsive,
    };

    Q_ENUM_NS(Orientation)

    // Start / End 为逻辑方向，RTL 下左右互换
    enum ControlPosition
    {
        Start = 0,
        End,
    };

    Q_ENUM_NS(ControlPosition)

    enum LegendVariant
    {
        Legend = 0,
        Label,
    };

    Q_ENUM_NS(LegendVariant)
} // namespace ShadFieldType

namespace ShadInputDialogType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum InputMode
    {
        TextInput = 0,
        MultiLineTextInput,
        IntInput,
        DoubleInput,
    };

    Q_ENUM_NS(InputMode)
} // namespace ShadInputDialogType

namespace ShadAlertDialogType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Size
    {
        Medium = 0,
        Small,
    };

    Q_ENUM_NS(Size)

    enum MediaVariant
    {
        Default = 0,
        Destructive,
    };

    Q_ENUM_NS(MediaVariant)
} // namespace ShadAlertDialogType

namespace ShadItemType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Variant
    {
        Default = 0,
        Outline,
        Muted,
    };

    Q_ENUM_NS(Variant)

    enum Size
    {
        Medium = 0,
        Small,
        ExtraSmall,
    };

    Q_ENUM_NS(Size)
} // namespace ShadItemType

namespace ShadColorPickerType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Mode
    {
        Rgb = 0,
        Hsv,
    };

    Q_ENUM_NS(Mode)
} // namespace ShadColorPickerType

namespace ShadTableType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum ImageShape
    {
        Rounded = 0,
        Circle,
    };

    Q_ENUM_NS(ImageShape)
} // namespace ShadTableType

namespace ShadChartType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum CurveType
    {
        Natural = 0,
        Linear,
        Step,
    };

    Q_ENUM_NS(CurveType)

    enum StackMode
    {
        NoStack = 0,
        Stacked,
        Expanded,
    };

    Q_ENUM_NS(StackMode)

    enum FillStyle
    {
        Solid = 0,
        Gradient,
    };

    Q_ENUM_NS(FillStyle)

    enum TooltipIndicator
    {
        Dot = 0,
        Line,
        Dashed,
    };

    Q_ENUM_NS(TooltipIndicator)

    // Metal 只在 macOS / iOS 上可选
    enum RenderApi
    {
        Auto = 0,
        Software,
        OpenGL,
        Direct3D11,
        Direct3D12,
        Vulkan,
        Metal,
    };

    Q_ENUM_NS(RenderApi)

    // K 线涨跌配色，红色 #ef4444，绿色 #22c55e
    enum CandleColorScheme
    {
        RedRising = 0,
        GreenRising,
    };

    Q_ENUM_NS(CandleColorScheme)
} // namespace ShadChartType

namespace ShadEmptyType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    // 媒体区样式：Default 只显示图标，Icon 带底色块
    enum MediaVariant
    {
        Default = 0,
        Icon,
    };

    Q_ENUM_NS(MediaVariant)
} // namespace ShadEmptyType

namespace ShadTreeSelectType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    // 多选时回显哪些勾选节点：叶子节点、全选时的父节点或全部节点
    enum CheckedStrategy
    {
        ShowChild = 0,
        ShowParent,
        ShowAll,
    };

    Q_ENUM_NS(CheckedStrategy)
} // namespace ShadTreeSelectType

namespace ShadDescriptionsType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    // 标签在内容旁边或上方
    enum Layout
    {
        Horizontal = 0,
        Vertical,
    };

    Q_ENUM_NS(Layout)

    enum Size
    {
        Medium = 0,
        Small,
    };

    Q_ENUM_NS(Size)
} // namespace ShadDescriptionsType

namespace ShadToolbarType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum ButtonStyle
    {
        IconOnly = 0,
        TextBesideIcon,
        TextOnly,
    };

    Q_ENUM_NS(ButtonStyle)
} // namespace ShadToolbarType

namespace ShadPasskeyType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Error
    {
        Cancelled = 0,
        NotSupported,
        Timeout,
        DomainNotAllowed,
        InvalidOptions,
        Other,
    };

    Q_ENUM_NS(Error)

    enum UserVerification
    {
        Preferred = 0,
        Required,
        Discouraged,
    };

    Q_ENUM_NS(UserVerification)
}

namespace ShadBiometricType
{
    Q_NAMESPACE_EXPORT (SHADCN_EXPORT)

    enum Policy
    {
        Biometrics = 0,
        DeviceOwner,
    };

    Q_ENUM_NS(Policy)

    enum Method
    {
        Unavailable = 0,
        Fingerprint,
        Face,
        Password,
    };

    Q_ENUM_NS(Method)

    enum Error
    {
        Cancelled = 0,
        NotSupported,
        NotEnrolled,
        Lockout,
        Failed,
        Other,
    };

    Q_ENUM_NS(Error)
}

#endif // SHADDEF_H

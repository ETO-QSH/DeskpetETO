from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtWidgets import QWidget, QFileDialog

from DeskpetETO.FileListSettingCard import FileListSettingCard
from qfluentwidgets import (
    SettingCardGroup, SwitchSettingCard, PushSettingCard, FluentIcon, ConfigItem, ComboBoxSettingCard,
    ScrollArea, ExpandLayout, Theme, InfoBar, setTheme, setThemeColor, isDarkTheme, QConfig,
    FolderValidator, BoolValidator, OptionsValidator, OptionsConfigItem, qconfig, RangeConfigItem, RangeValidator
)

from DeskpetETO.ColorPicker import ColorSettingCard
from DeskpetETO.FloatSettingCard import RangeSettingCard2F, RangeValidator2F, RangeSettingCardInt


class Config(QConfig):
    """ Config of application """
    downloadFolder = ConfigItem("Personalization", "Folders", "./output", FolderValidator())
    wareHouse = OptionsConfigItem(
        "Personalization", "WareHouse", "仓库",
        OptionsValidator(["仓库", "用户"])
    )
    dpiScale = OptionsConfigItem(
        "Personalization", "DpiScale", "Auto",
        OptionsValidator([1, 1.25, 1.5, 1.75, 2, "Auto"]), restart=True
    )
    firstPage = OptionsConfigItem(
        "Personalization", "Page", "HomeInterface", OptionsValidator([
            "HomeInterface", "CardInterface", "DownloadInterface", "SettingInterface", "DocumentInterface"
        ])
    )

    minimizeToTray = ConfigItem("Configuration", "Minimize", True, BoolValidator())
    automateStartUp = ConfigItem("Configuration", "Startup", False, BoolValidator())
    checkUpdateAtStartUp = ConfigItem("Configuration", "Update", False, BoolValidator())
    penetrateClickAll = ConfigItem("Configuration", "Unclick", False, BoolValidator())
    keepWindowTop = ConfigItem("Configuration", "Keeper", True, BoolValidator())

    gravity = RangeConfigItem("Animation", "Gravity", 3.0, RangeValidator2F(0.0, 6.0))
    smooth = RangeConfigItem("Animation", "Smooth", 0.35, RangeValidator2F(0.0, 0.5))
    velocity = RangeConfigItem("Animation", "Velocity", 5, RangeValidator(0, 10))
    dynamic = RangeConfigItem("Animation", "Dynamic", 2, RangeValidator(0, 6))
    modelSize = RangeConfigItem("Animation", "ModelSize", 0.0, RangeValidator2F(-2.0, 2.0))


cfg = Config()
qconfig.load('./config/config.json', cfg)


class Setting(ScrollArea):
    """ Setting interface """
    checkUpdateSig = pyqtSignal()
    minimizeToTrayChanged = pyqtSignal(bool)
    automateStartUpChanged = pyqtSignal(bool)

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        self.scrollWidget = QWidget()
        self.setObjectName("SettingInterface")
        self.expandLayout = ExpandLayout(self.scrollWidget)

        # personalization
        self.personalGroup = SettingCardGroup(self.tr('Personalization'), self.scrollWidget)

        self.themeCard = ComboBoxSettingCard(
            cfg.themeMode,
            FluentIcon.CONSTRACT,
            self.tr('黑白主题'),
            self.tr('切换日间或夜间主题，自动为跟随系统设置'),
            texts=[self.tr('Light'), self.tr('Dark'), self.tr('Auto')],
            parent=self.personalGroup
        )

        self.themeColorCard = ColorSettingCard(
            cfg.themeColor,
            FluentIcon.PALETTE,
            self.tr('主题颜色'),
            self.tr('设置控件的主题颜色'),
            self.personalGroup,
            False,
            False
        )
        self.__updateThemeColorDescription()  # 初始化时更新描述
        self.themeColorCard.colorPicker.setMaximumWidth(84)
        self.themeColorCard.colorPicker.setMinimumWidth(84)

        self.zoomCard = ComboBoxSettingCard(
            cfg.dpiScale,
            FluentIcon.ZOOM,
            self.tr("应用缩放"),
            self.tr('设置软件缩放DPI，自动为跟随系统设置'),
            texts=["100%", "125%", "150%", "175%", "200%", self.tr("Auto")],
            parent=self.personalGroup
        )

        self.warehouseCard = ComboBoxSettingCard(
            cfg.wareHouse,
            FluentIcon.DICTIONARY,
            self.tr("主导数据"),
            self.tr('设置键名重复时，优先选择的模型源'),
            texts=["仓库", "用户"],
            parent=self.personalGroup
        )

        self.downloadFolderCard = PushSettingCard(
            self.tr('更改'),
            FluentIcon.FOLDER,
            self.tr("输出目录"),
            self.tr("资源下载的临时目录"),
            parent=self.personalGroup
        )
        self.__updateDownloadFolderDescription()  # 初始化时更新描述
        self.downloadFolderCard.button.setMaximumWidth(84)
        self.downloadFolderCard.button.setMinimumWidth(84)
        self.downloadFolderCard.button.setStyleSheet("padding: 5px 0px;")

        self.pageCard = ComboBoxSettingCard(
            cfg.firstPage,
            FluentIcon.FLAG,
            self.tr("启动首页"),
            self.tr('设置软件启动时，显现的初始页面'),
            texts=["主页", "管理", "下载", "设置", "文档"],
            parent=self.personalGroup
        )

        # configuration
        self.otherGroup = SettingCardGroup(self.tr('Configuration'), self.scrollWidget)

        self.minimizeToTrayCard = SwitchSettingCard(
            FluentIcon.MINIMIZE,
            self.tr('最小化托盘'),
            self.tr('关闭时最小化至托盘后台监控'),
            configItem=cfg.minimizeToTray,
            parent=self.otherGroup
        )
        self.cnSwitchButton(self.minimizeToTrayCard.switchButton)

        self.automateStartUpCard = SwitchSettingCard(
            FluentIcon.POWER_BUTTON,
            self.tr('开机自启动'),
            self.tr('将软件快捷方式加入开机自启目录'),
            configItem=cfg.automateStartUp,
            parent=self.otherGroup
        )
        self.cnSwitchButton(self.automateStartUpCard.switchButton)

        self.updateOnStartUpCard = SwitchSettingCard(
            FluentIcon.UPDATE,
            self.tr('自检查更新'),
            self.tr('使软件在启动时自检最新仓库获取资源和版本信息'),
            configItem=cfg.checkUpdateAtStartUp,
            parent=self.otherGroup
        )
        self.cnSwitchButton(self.updateOnStartUpCard.switchButton)

        self.penetrateClickCard = SwitchSettingCard(
            FluentIcon.TAG,
            self.tr('点击全穿透'),
            self.tr('半透明桌宠并使所有鼠标操作透过'),
            configItem=cfg.penetrateClickAll,
            parent=self.otherGroup
        )
        self.cnSwitchButton(self.penetrateClickCard.switchButton)

        self.keepWindowTopCard = SwitchSettingCard(
            FluentIcon.PIN,
            self.tr('窗口置顶化'),
            self.tr('半透明桌宠并使所有鼠标操作透过'),
            configItem=cfg.keepWindowTop,
            parent=self.otherGroup
        )
        self.cnSwitchButton(self.keepWindowTopCard.switchButton)

        # animation
        self.animateGroup = SettingCardGroup(self.tr('Animation'), self.scrollWidget)

        self.gravityCard = RangeSettingCard2F(
            cfg.gravity,
            FluentIcon.DOWN,
            self.tr('重力加速度'),
            self.tr('设置启用下落时的下落加速度折算时间（s）'),
            2,
            parent=self.animateGroup
        )

        self.smoothCard = RangeSettingCard2F(
            cfg.smooth,
            FluentIcon.STOP_WATCH,
            self.tr('平滑混合时间'),
            self.tr('设置两个不同动画切换时的混合时长（s）'),
            20,
            parent=self.animateGroup
        )

        self.velocityCard = RangeSettingCardInt(
            cfg.velocity,
            FluentIcon.SPEED_HIGH,
            self.tr('移动速度'),
            self.tr('设置模型在移动时的速度期望（20px/s）'),
            parent=self.animateGroup
        )

        self.dynamicCard = RangeSettingCardInt(
            cfg.dynamic,
            FluentIcon.BASKETBALL,
            self.tr('动作活跃度'),
            self.tr('活跃度越高越容易触发行走以及特殊动画（int）'),
            parent=self.animateGroup,
        )

        self.modelSizeCard = RangeSettingCard2F(
            cfg.modelSize,
            FluentIcon.FIT_PAGE,
            self.tr('缩放比例'),
            self.tr('设置模型相对于原尺寸的缩放比例（2^n）'),
            5,
            parent=self.animateGroup
        )

        self.__initWidget()
        self.setObjectName('SettingInterface')

    def __initWidget(self):
        self.resize(540, 810)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.setViewportMargins(0, 0, 0, 0)
        self.setWidget(self.scrollWidget)
        self.setWidgetResizable(True)

        # initialize style sheet
        self.__setQss()

        # initialize layout
        self.__initLayout()
        self.__connectSignalToSlot()

    def __initLayout(self):
        self.personalGroup.addSettingCard(self.themeCard)
        self.personalGroup.addSettingCard(self.themeColorCard)
        self.personalGroup.addSettingCard(self.pageCard)
        self.personalGroup.addSettingCard(self.warehouseCard)
        self.personalGroup.addSettingCard(self.zoomCard)
        self.personalGroup.addSettingCard(self.downloadFolderCard)

        self.otherGroup.addSettingCard(self.penetrateClickCard)
        self.otherGroup.addSettingCard(self.keepWindowTopCard)
        self.otherGroup.addSettingCard(self.updateOnStartUpCard)
        self.otherGroup.addSettingCard(self.minimizeToTrayCard)
        self.otherGroup.addSettingCard(self.automateStartUpCard)

        self.animateGroup.addSettingCard(self.gravityCard)
        self.animateGroup.addSettingCard(self.smoothCard)
        self.animateGroup.addSettingCard(self.velocityCard)
        self.animateGroup.addSettingCard(self.dynamicCard)
        self.animateGroup.addSettingCard(self.modelSizeCard)

        # add setting card group to layout
        self.expandLayout.setSpacing(20)
        self.expandLayout.setContentsMargins(10, 0, 10, 15)
        self.expandLayout.addWidget(self.personalGroup)
        self.expandLayout.addWidget(self.otherGroup)
        self.expandLayout.addWidget(self.animateGroup)

    def __setQss(self):
        """ set style sheet """
        self.scrollWidget.setObjectName('scrollWidget')

        theme = 'dark' if isDarkTheme() else 'light'
        with open(f'./config/QSS/{theme}/setting.qss', encoding='utf-8') as f:
            self.setStyleSheet(f.read())

    def __showRestartTooltip(self):
        """ show restart tooltip """
        InfoBar.warning(
            '',
            self.tr('该配置在重启之后生效喵'),
            parent=self.window()
        )

    def __onDownloadFolderCardClicked(self):
        """ download folder card clicked slot """
        folder = QFileDialog.getExistingDirectory(
            self, self.tr("选择文件夹"), "./")
        if not folder or cfg.get(cfg.downloadFolder) == folder:
            return

        cfg.set(cfg.downloadFolder, folder)
        self.downloadFolderCard.setContent(folder)

    def __onThemeChanged(self, theme: Theme):
        """ theme changed slot """
        setTheme(theme)
        self.__setQss()

    def __connectSignalToSlot(self):
        """ connect signal to slot """
        cfg.appRestartSig.connect(self.__showRestartTooltip)
        cfg.themeChanged.connect(self.__onThemeChanged)

        self.themeColorCard.colorChanged.connect(setThemeColor)
        self.downloadFolderCard.clicked.connect(self.__onDownloadFolderCardClicked)
        self.minimizeToTrayCard.checkedChanged.connect(self.minimizeToTrayChanged)
        self.automateStartUpCard.checkedChanged.connect(self.automateStartUpChanged)

        cfg.themeColor.valueChanged.connect(self.__updateThemeColorDescription)
        cfg.downloadFolder.valueChanged.connect(self.__updateDownloadFolderDescription)

    def __updateThemeColorDescription(self):
        """ 更新主题颜色描述 """
        text = self.tr(f'设置控件的主题颜色，当前颜色：{cfg.themeColor.value.name().upper()}')
        self.themeColorCard.contentLabel.setText(text)

    def __updateDownloadFolderDescription(self):
        text = self.tr(f'资源下载的临时目录，目前目录：\n{cfg.downloadFolder.value}')
        self.downloadFolderCard.contentLabel.setText(text)

    def cnSwitchButton(self, switch):
        # 设置文本
        switch.setOnText("开启")
        switch.setOffText("关闭")
        # 通过信号强制刷新
        switch.checkedChanged.connect(lambda: switch.setText(switch.onText if switch.isChecked() else switch.offText))

from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtWidgets import QWidget, QFileDialog

from qfluentwidgets import (
    SettingCardGroup, SwitchSettingCard, PushSettingCard, FluentIcon, ConfigItem, ComboBoxSettingCard,
    ScrollArea, ExpandLayout, Theme, InfoBar, setTheme, setThemeColor, isDarkTheme, QConfig,
    FolderValidator, BoolValidator, OptionsValidator, OptionsConfigItem, qconfig
)

from DeskpetETO.ColorPicker import ColorSettingCard


class Config(QConfig):
    """ Config of application """
    downloadFolder = ConfigItem("MainWindow", "Folders", "./output", FolderValidator())
    minimizeToTray = ConfigItem("MainWindow", "Minimize", True, BoolValidator())
    automateStartUp = ConfigItem("MainWindow", "Startup", False, BoolValidator())
    checkUpdateAtStartUp = ConfigItem("MainWindow", "Update", False, BoolValidator())
    dpiScale = OptionsConfigItem(
        "MainWindow", "DpiScale", "Auto",
        OptionsValidator([1, 1.25, 1.5, 1.75, 2, "Auto"]), restart=True
    )


cfg = Config()
qconfig.load('./config/config.json', cfg)


class Setting(ScrollArea):
    """ Setting interface """
    checkUpdateSig = pyqtSignal()
    musicFoldersChanged = pyqtSignal(list)
    downloadFolderChanged = pyqtSignal(str)
    minimizeToTrayChanged = pyqtSignal(bool)
    automateStartUpChanged = pyqtSignal(bool)

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        self.scrollWidget = QWidget()
        self.expandLayout = ExpandLayout(self.scrollWidget)

        # personalization
        self.personalGroup = SettingCardGroup(self.tr('Personalization'), self.scrollWidget)

        self.themeCard = ComboBoxSettingCard(
            cfg.themeMode,
            FluentIcon.BRUSH,
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

        # configuration
        self.otherGroup = SettingCardGroup(self.tr('Configuration'), self.scrollWidget)

        self.downloadFolderCard = PushSettingCard(
            self.tr('打开'),
            FluentIcon.DOWNLOAD,
            self.tr("输出文件夹"),
            self.tr("资源下载的临时目录，目前目录"),
            parent=self.otherGroup
        )
        self.__updateDownloadFolderDescription()  # 初始化时更新描述
        self.downloadFolderCard.button.setMaximumWidth(84)
        self.downloadFolderCard.button.setMinimumWidth(84)
        self.downloadFolderCard.button.setStyleSheet("padding: 5px 0px;")

        self.minimizeToTrayCard = SwitchSettingCard(
            FluentIcon.MINIMIZE,
            self.tr('最小化托盘'),
            self.tr('关闭时最小化至托盘后台监控'),
            configItem=cfg.minimizeToTray,
            parent=self.otherGroup
        )
        switch_1 = self.minimizeToTrayCard.switchButton
        # 设置文本
        switch_1.setOnText("开启")
        switch_1.setOffText("关闭")
        # 通过信号强制刷新
        switch_1.checkedChanged.connect(lambda: switch_1.setText(switch_1.onText if switch_1.isChecked() else switch_1.offText))

        self.automateStartUpCard = SwitchSettingCard(
            FluentIcon.POWER_BUTTON,
            self.tr('开机自启动'),
            self.tr('将软件快捷方式加入开机自启目录'),
            configItem=cfg.automateStartUp,
            parent=self.otherGroup
        )
        switch_2 = self.automateStartUpCard.switchButton
        # 设置文本
        switch_2.setOnText("开启")
        switch_2.setOffText("关闭")
        # 通过信号强制刷新
        switch_2.checkedChanged.connect(lambda: switch_2.setText(switch_2.onText if switch_2.isChecked() else switch_2.offText))

        self.updateOnStartUpCard = SwitchSettingCard(
            FluentIcon.UPDATE,
            self.tr('自检查更新'),
            self.tr('使软件在启动时自检最新仓库获取资源和版本信息'),
            configItem=cfg.checkUpdateAtStartUp,
            parent=self.otherGroup
        )
        switch_3 = self.updateOnStartUpCard.switchButton
        # 设置文本
        switch_3.setOnText("开启")
        switch_3.setOffText("关闭")
        # 通过信号强制刷新
        switch_3.checkedChanged.connect(lambda: switch_3.setText(switch_3.onText if switch_3.isChecked() else switch_3.offText))

        self.__initWidget()
        self.setObjectName('SettingInterface')  # 添加对象标识

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
        self.personalGroup.addSettingCard(self.zoomCard)

        self.otherGroup.addSettingCard(self.downloadFolderCard)
        self.otherGroup.addSettingCard(self.updateOnStartUpCard)
        self.otherGroup.addSettingCard(self.minimizeToTrayCard)
        self.otherGroup.addSettingCard(self.automateStartUpCard)

        # add setting card group to layout
        self.expandLayout.setSpacing(20)
        self.expandLayout.setContentsMargins(10, 15, 10, 15)
        self.expandLayout.addWidget(self.personalGroup)
        self.expandLayout.addWidget(self.otherGroup)

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
        # change the theme of qfluentwidgets
        setTheme(theme)

        # chang the theme of setting interface
        self.__setQss()

    def __connectSignalToSlot(self):
        """ connect signal to slot """
        cfg.appRestartSig.connect(self.__showRestartTooltip)
        cfg.themeChanged.connect(self.__onThemeChanged)

        self.themeColorCard.colorChanged.connect(setThemeColor)
        self.downloadFolderCard.clicked.connect(self.__onDownloadFolderCardClicked)
        self.minimizeToTrayCard.checkedChanged.connect(self.minimizeToTrayChanged)
        self.minimizeToTrayCard.checkedChanged.connect(self.automateStartUpChanged)

        cfg.themeColor.valueChanged.connect(self.__updateThemeColorDescription)
        cfg.downloadFolder.valueChanged.connect(self.__updateDownloadFolderDescription)

    def __updateThemeColorDescription(self):
        """ 更新主题颜色描述 """
        text = self.tr(f'设置控件的主题颜色，当前颜色：{cfg.themeColor.value.name().upper()}')
        self.themeColorCard.contentLabel.setText(text)

    def __updateDownloadFolderDescription(self):
        text = self.tr(f'资源下载的临时目录，目前目录：\n{cfg.downloadFolder.value}')
        self.downloadFolderCard.contentLabel.setText(text)

from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtWidgets import QWidget, QFileDialog

from qfluentwidgets import (
    SettingCardGroup, SwitchSettingCard, OptionsSettingCard, PushSettingCard, FluentIcon, ConfigItem,
    ColorSettingCard, ScrollArea, ExpandLayout, Theme, InfoBar, setTheme, setThemeColor, isDarkTheme,
    FolderValidator, BoolValidator, OptionsValidator, OptionsConfigItem, qconfig, QConfig
)


class Config(QConfig):
    """ Config of application """
    downloadFolder = ConfigItem("MainWindow", "Folders", "../output", FolderValidator())
    minimizeToTray = ConfigItem("MainWindow", "Minimize", True, BoolValidator())
    automateStartUp = ConfigItem("MainWindow", "Startup", False, BoolValidator())
    checkUpdateAtStartUp = ConfigItem("MainWindow", "Update", False, BoolValidator())
    dpiScale = OptionsConfigItem(
        "MainWindow", "DpiScale", "Auto",
        OptionsValidator([1, 1.25, 1.5, 1.75, 2, "Auto"]), restart=True
    )


cfg = Config()
qconfig.load('config.json', cfg)


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
        self.themeCard = OptionsSettingCard(
            cfg.themeMode,
            FluentIcon.BRUSH,
            self.tr('黑白主题'),
            texts=[self.tr('Light'), self.tr('Dark'), self.tr('Auto')],
            parent=self.personalGroup
        )
        self.themeColorCard = ColorSettingCard(
            cfg.themeColor,
            FluentIcon.PALETTE,
            self.tr('主题颜色'),
            None,
            self.personalGroup
        )
        self.zoomCard = OptionsSettingCard(
            cfg.dpiScale,
            FluentIcon.ZOOM,
            self.tr("应用缩放"),
            texts=["100%", "125%", "150%", "175%", "200%", self.tr("Auto")],
            parent=self.personalGroup
        )

        # configuration
        self.otherGroup = SettingCardGroup(self.tr('Configuration'), self.scrollWidget)
        self.downloadFolderCard = PushSettingCard(
            self.tr('打开'),
            FluentIcon.DOWNLOAD,
            self.tr("输出文件夹"),
            parent=self.otherGroup
        )
        self.minimizeToTrayCard = SwitchSettingCard(
            FluentIcon.MINIMIZE,
            self.tr('最小化托盘'),
            configItem=cfg.minimizeToTray,
            parent=self.otherGroup
        )
        self.automateStartUpCard = SwitchSettingCard(
            FluentIcon.POWER_BUTTON,
            self.tr('开机自启动'),
            configItem=cfg.automateStartUp,
            parent=self.otherGroup
        )
        self.updateOnStartUpCard = SwitchSettingCard(
            FluentIcon.UPDATE,
            self.tr('自检查更新'),
            configItem=cfg.checkUpdateAtStartUp,
            parent=self.otherGroup
        )

        self.__initWidget()

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
        self.expandLayout.setSpacing(25)
        self.expandLayout.setContentsMargins(20, 15, 20, 15)
        self.expandLayout.addWidget(self.personalGroup)
        self.expandLayout.addWidget(self.otherGroup)

    def __setQss(self):
        """ set style sheet """
        self.scrollWidget.setObjectName('scrollWidget')

        theme = 'dark' if isDarkTheme() else 'light'
        with open(f'resource/qss/{theme}/setting_interface.qss', encoding='utf-8') as f:
            self.setStyleSheet(f.read())

    def __showRestartTooltip(self):
        """ show restart tooltip """
        print('okokok')
        InfoBar.warning(
            '',
            self.tr('Configuration takes effect after restart'),
            parent=self.window()
        )

    def __onDownloadFolderCardClicked(self):
        """ download folder card clicked slot """
        folder = QFileDialog.getExistingDirectory(
            self, self.tr("Choose folder"), "./")
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

        self.downloadFolderCard.clicked.connect(self.__onDownloadFolderCardClicked)
        self.themeColorCard.colorChanged.connect(setThemeColor)
        self.minimizeToTrayCard.checkedChanged.connect(self.minimizeToTrayChanged)
        self.minimizeToTrayCard.checkedChanged.connect(self.automateStartUpChanged)

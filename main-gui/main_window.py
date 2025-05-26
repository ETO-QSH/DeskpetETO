import os
import sys

from PyQt5 import QtGui, QtCore
from PyQt5.QtCore import Qt, QUrl
from PyQt5.QtGui import QDesktopServices
from PyQt5.QtWidgets import QApplication, QFrame, QHBoxLayout

from DeskpetETO.CardFrame import CardWindow
from DeskpetETO.Document import Document
from DeskpetETO.MessageBox import CustomMessageBox
from DeskpetETO.Setting import Setting, cfg

from qfluentwidgets import (
    NavigationItemPosition, setTheme, Theme, MSFluentWindow, SubtitleLabel,
    setFont, qconfig, setThemeColor, isDarkTheme, FluentIcon
)


class Widget(QFrame):
    def __init__(self, text: str, parent=None):
        super().__init__(parent=parent)
        self.label = SubtitleLabel(text, self)
        self.hBoxLayout = QHBoxLayout(self)

        setFont(self.label, 24)
        self.label.setAlignment(Qt.AlignCenter)
        self.hBoxLayout.addWidget(self.label, 1, Qt.AlignCenter)
        self.setObjectName(text.replace(' ', '-'))


class Window(MSFluentWindow):
    def __init__(self):
        super().__init__()

        self.homeInterface = Widget('HomeInterface', self)
        self.cardInterface = CardWindow()
        self.downloadInterface = Widget('DownloadInterface', self)

        self.settingInterface = Setting(self)
        self.documentInterface = Document(self)

        self.initNavigation()
        self.initWindow()

        qconfig.load('./config/config.json', cfg)

        # 应用初始主题
        self.setQss()
        setTheme(cfg.get(cfg.themeMode))
        setThemeColor(cfg.get(cfg.themeColor))

        # 添加主题变化监听
        cfg.themeChanged.connect(self._onThemeChanged)
        cfg.themeColorChanged.connect(self._onThemeColorChanged)

    def closeEvent(self, event):
        # 关闭时保存配置
        qconfig.save()
        super().closeEvent(event)

    def _onThemeChanged(self, theme: Theme):
        setTheme(theme)
        self.setQss()

    def _onThemeColorChanged(self, color):
        setThemeColor(color)
        self.setQss()

    def setQss(self):
        theme = 'dark' if isDarkTheme() else 'light'
        with open(f'./config/QSS/{theme}/demo.qss', encoding='utf-8') as f:
            self.setStyleSheet(f.read())

    def initNavigation(self):
        self.addSubInterface(self.homeInterface, FluentIcon.HOME, '主页', FluentIcon.HOME)
        self.addSubInterface(self.cardInterface, FluentIcon.LABEL, '管理', FluentIcon.LABEL)
        self.addSubInterface(self.downloadInterface, FluentIcon.DOWNLOAD, '下载', FluentIcon.DOWNLOAD)

        self.addSubInterface(
            self.settingInterface, FluentIcon.SETTING, '设置',
            FluentIcon.SETTING, NavigationItemPosition.BOTTOM
        )
        self.addSubInterface(
            self.documentInterface, FluentIcon.HELP, '文档',
            FluentIcon.HELP, NavigationItemPosition.BOTTOM
        )
        self.navigationInterface.addItem(
            routeKey='Github',
            icon=FluentIcon.GITHUB,
            text='源码',
            onClick=self.showMessage,
            selectable=False,
            position=NavigationItemPosition.BOTTOM,
        )

        match cfg.get(cfg.firstPage):
            case 'HomeInterface':
                self.switchTo(self.homeInterface)
            case 'CardInterface':
                self.switchTo(self.cardInterface)
            case 'DownloadInterface':
                self.switchTo(self.downloadInterface)
            case 'SettingInterface':
                self.switchTo(self.settingInterface)
            case 'DocumentInterface':
                self.switchTo(self.documentInterface)

    def initWindow(self):
        self.setGeometry(QtCore.QRect(0, 0, 610, 840))

        self.titleBar.maxBtn.hide()
        self.titleBar.setDoubleClickEnabled(False)

        icon = QtGui.QIcon(r".\resource\Sprite-0001.ico")
        self.titleBar.iconLabel.setPixmap(icon.pixmap(25, 25))
        self.titleBar.iconLabel.setFixedSize(36, 36)

        self.titleBar.hBoxLayout.insertSpacing(0, 4)

        self.titleBar.titleLabel.setText('DeskpetETO')

        titleLabelStyle = """
        QLabel {
            font-family: '萝莉体';
            font-size: 18px;
        }
        """
        self.titleBar.titleLabel.setStyleSheet(titleLabelStyle)

        desktop = QApplication.desktop().availableGeometry()
        w, h = desktop.width(), desktop.height()
        self.move(w//2 - self.width()//2, h//2 - self.height()//2)

    def showMessage(self):
        agent_list = ["Ace", "SilverAsh", "Exusiai"]
        skin_list = ["冬季制服", "战斗装备", "休闲装扮"]
        brand_list = ["战术装备", "时尚品牌", "科幻系列"]
        model_list = ["3D模型", "2D立绘", "Q版造型"]

        w = CustomMessageBox(
            self.stackedWidget, agent_list, skin_list, brand_list, model_list
        )
        w.yesButton.setText("确认")
        w.cancelButton.setText("取消")
        if w.exec():
            print(w.getInputs())

    def toGithub(self):
        QDesktopServices.openUrl(QUrl("https://github.com/ETO-QSH/DeskpetETO"))


if __name__ == '__main__':
    # QApplication.setHighDpiScaleFactorRoundingPolicy(Qt.HighDpiScaleFactorRoundingPolicy.PassThrough)
    # QApplication.setAttribute(Qt.AA_EnableHighDpiScaling)
    # QApplication.setAttribute(Qt.AA_UseHighDpiPixmaps)

    # enable dpi scale
    if cfg.get(cfg.dpiScale) == "Auto":
        QApplication.setHighDpiScaleFactorRoundingPolicy(
            Qt.HighDpiScaleFactorRoundingPolicy.PassThrough)
        QApplication.setAttribute(Qt.AA_EnableHighDpiScaling)
    else:
        os.environ["QT_ENABLE_HIGHDPI_SCALING"] = "0"
        os.environ["QT_SCALE_FACTOR"] = str(cfg.get(cfg.dpiScale))

    QApplication.setAttribute(Qt.AA_UseHighDpiPixmaps)

    app = QApplication(sys.argv)
    app.setAttribute(Qt.AA_DontCreateNativeWidgetSiblings)

    w = Window()
    w.show()
    app.exec_()

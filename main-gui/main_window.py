# coding:utf-8
import sys

from PyQt5 import QtGui
from PyQt5.QtCore import Qt, QUrl
from PyQt5.QtGui import QIcon, QDesktopServices, QFont
from PyQt5.QtWidgets import QApplication, QFrame, QHBoxLayout
from qfluentwidgets import (NavigationItemPosition, MessageBox, setTheme, Theme, MSFluentWindow,
                            NavigationAvatarWidget, qrouter, SubtitleLabel, setFont)
from qfluentwidgets import FluentIcon as FIF


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

        self.homeInterface = Widget('Home Interface', self)
        self.cardInterface = Widget('Card Interface', self)
        self.downloadInterface = Widget('Download Interface', self)
        self.moreInterface = Widget('More Interface', self)

        self.settingButton = Widget('Setting Interface', self)
        self.documentInterface = Widget('Document Interface', self)
        self.certificateButton = Widget('Certificate Interface', self)

        self.initNavigation()
        self.initWindow()

    def initNavigation(self):
        self.addSubInterface(self.homeInterface, FIF.HOME, '主页', FIF.HOME)
        self.addSubInterface(self.cardInterface, FIF.LABEL, '管理', FIF.LABEL)
        self.addSubInterface(self.downloadInterface, FIF.DOWNLOAD, '下载', FIF.DOWNLOAD)
        self.addSubInterface(self.moreInterface, FIF.APPLICATION, '更多', FIF.APPLICATION)

        self.addSubInterface(
            self.settingButton, FIF.SETTING, '设置',
            FIF.SETTING, NavigationItemPosition.BOTTOM
        )
        self.addSubInterface(
            self.documentInterface, FIF.HELP, '文档',
            FIF.HELP, NavigationItemPosition.BOTTOM
        )
        self.addSubInterface(
            self.certificateButton, FIF.CERTIFICATE, '许可',
            FIF.CERTIFICATE, NavigationItemPosition.BOTTOM
        )

        self.navigationInterface.addItem(
            routeKey='Github',
            icon=FIF.GITHUB,
            text='源码',
            onClick=self.showMessageBox,
            selectable=False,
            position=NavigationItemPosition.BOTTOM,
        )

        self.navigationInterface.setCurrentItem(self.homeInterface.objectName())

    def initWindow(self):
        self.resize(480, 640)

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
            font-size: 15px;
        }
        """
        self.titleBar.titleLabel.setStyleSheet(titleLabelStyle)

        desktop = QApplication.desktop().availableGeometry()
        w, h = desktop.width(), desktop.height()
        self.move(w//2 - self.width()//2, h//2 - self.height()//2)

    def showMessageBox(self):
        w = MessageBox(
            'byETO',
            '跳转至项目GitHub首页，可以为本项目提交Issue哦~',
            self
        )
        w.yesButton.setText('这就去')
        w.cancelButton.setText('懒得看')

        self.set_font("萝莉体", 10, w.yesButton)

        titleLabelStyle = """
          QLabel {
              font-family: '萝莉体';
              font-size: 20px;
          }
        """
        w.titleLabel.setStyleSheet(titleLabelStyle)

        contentLabelStyle = """
          QLabel {
              font-family: '萝莉体';
              font-size: 16px;
          }
        """
        w.contentLabel.setStyleSheet(contentLabelStyle)

        cancelButtonStyle = """
          QPushButton {
              font-family: '萝莉体';
              font-size: 13px;
          }
        """
        w.cancelButton.setStyleSheet(cancelButtonStyle)

        if w.exec():
            QDesktopServices.openUrl(QUrl("https://github.com/ETO-QSH/DeskpetETO"))

    def set_font(self, font_name, font_size, label):
        """ 设置指定标签的字体和大小 """
        font = QFont(font_name, font_size)
        label.setFont(font)


if __name__ == '__main__':
    QApplication.setHighDpiScaleFactorRoundingPolicy(Qt.HighDpiScaleFactorRoundingPolicy.PassThrough)
    QApplication.setAttribute(Qt.AA_EnableHighDpiScaling)
    QApplication.setAttribute(Qt.AA_UseHighDpiPixmaps)

    # setTheme(Theme.DARK)

    app = QApplication(sys.argv)
    w = Window()
    w.show()
    app.exec_()

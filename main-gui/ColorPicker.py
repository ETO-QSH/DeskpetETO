from typing import Union

from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtGui import QColor, QIcon, QPainter
from PyQt5.QtWidgets import QToolButton, QColorDialog

from qfluentwidgets import qconfig, FluentIconBase, SettingCard, isDarkTheme, ColorDialog


class ColorPickerButton(QToolButton):
    """ Color picker button """

    colorChanged = pyqtSignal(QColor)

    def __init__(self, color: QColor, title: str, parent=None, enableAlpha=False, enable_realtime=False):
        super().__init__(parent=parent)
        self.title = title
        self.enableAlpha = enableAlpha
        self.enable_realtime = enable_realtime  # 新增控制参数
        self.setFixedSize(96, 32)
        self.setAttribute(Qt.WA_TranslucentBackground)

        self.setColor(color)
        self.setCursor(Qt.PointingHandCursor)
        self.clicked.connect(self.__showColorDialog)

    def __showColorDialog(self):
        """ show color dialog """
        # w = ColorDialog(self.color, self.tr(
        #     'Choose ')+self.title, self.window(), self.enableAlpha)
        # w.colorChanged.connect(self.__onColorChanged)
        # w.exec()

        # 创建原生颜色对话框
        dialog = QColorDialog(self.window())
        dialog.setWindowTitle(self.tr('Choose ') + self.title)
        dialog.setCurrentColor(self.color)

        # 连接颜色变化信号（如果需要实时预览）
        if self.enable_realtime:
            dialog.currentColorChanged.connect(self.__onColorChanged)

        # 显示对话框并获取结果
        if dialog.exec_():
            new_color = dialog.currentColor()
            if new_color.isValid() and new_color != self.color:
                self.__onColorChanged(new_color)

    def __onColorChanged(self, color):
        """ color changed slot """
        self.setColor(color)
        self.colorChanged.emit(color)

    def setColor(self, color):
        """ set color """
        self.color = QColor(color)
        self.update()

    def paintEvent(self, e):
        painter = QPainter(self)
        painter.setRenderHints(QPainter.Antialiasing)
        pc = QColor(255, 255, 255, 10) if isDarkTheme() else QColor(234, 234, 234)
        painter.setPen(pc)

        color = QColor(self.color)
        if not self.enableAlpha:
            color.setAlpha(255)

        painter.setBrush(color)
        painter.drawRoundedRect(self.rect().adjusted(1, 1, -1, -1), 5, 5)


class ColorSettingCard(SettingCard):
    """ Setting card with color picker """

    colorChanged = pyqtSignal(QColor)

    def __init__(self, configItem, icon: Union[str, QIcon, FluentIconBase], title: str,
                 content: str = None, parent=None, enableAlpha=False, enable_realtime=False):
        """
        Parameters
        ----------
        configItem: RangeConfigItem
            configuration item operated by the card

        icon: str | QIcon | FluentIconBase
            the icon to be drawn

        title: str
            the title of card

        content: str
            the content of card

        parent: QWidget
            parent widget

        enableAlpha: bool
            whether to enable the alpha channel
        """
        super().__init__(icon, title, content, parent)
        self.configItem = configItem
        self.enable_realtime = enable_realtime  # 新增控制参数
        self.colorPicker = ColorPickerButton(
            qconfig.get(configItem), title, self, enableAlpha, enable_realtime
        )
        self.hBoxLayout.addWidget(self.colorPicker, 0, Qt.AlignRight)
        self.hBoxLayout.addSpacing(16)
        self.colorPicker.colorChanged.connect(self.__onColorChanged)
        configItem.valueChanged.connect(self.setValue)

    def __onColorChanged(self, color: QColor):
        qconfig.set(self.configItem, color)
        self.colorChanged.emit(color)

    def setValue(self, color: QColor):
        self.colorPicker.setColor(color)
        qconfig.set(self.configItem, color)

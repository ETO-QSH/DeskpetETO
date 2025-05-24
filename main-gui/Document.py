from PyQt5.QtCore import Qt
from PyQt5.QtGui import QPixmap
from PyQt5.QtWidgets import QWidget, QHBoxLayout, QFrame
from qfluentwidgets import SmoothScrollArea, PixmapLabel, isDarkTheme, qconfig


class DocumentUI(SmoothScrollArea):
    def __init__(self, parent=None):
        super().__init__(parent=parent)
        qconfig.themeChanged.connect(self.updatePixmap)

        self.label = PixmapLabel(self)
        self.pixmap_light = QPixmap(r".\resource\help\light.jpeg")
        self.pixmap_dark = QPixmap(r".\resource\help\dark.jpeg")
        self.updatePixmap()
        self.setWidget(self.label)

    def updatePixmap(self):
        self.pixmap = self.pixmap_dark if isDarkTheme() else self.pixmap_light
        pixmapHeight = int(self.pixmap.height() * self.width() / self.pixmap.width())
        scaledPixmap = self.pixmap.scaled(self.width(), pixmapHeight, Qt.KeepAspectRatio, Qt.SmoothTransformation)
        self.label.setPixmap(scaledPixmap)

    def resizeEvent(self, event):
        super().resizeEvent(event)   # 调用基类的resizeEvent以保持其他功能正常
        pixmapWidth = self.width()
        pixmapHeight = int(self.pixmap.height() * pixmapWidth / self.pixmap.width())
        scaledPixmap = self.pixmap.scaled(pixmapWidth, pixmapHeight, Qt.KeepAspectRatio, Qt.SmoothTransformation)
        self.label.setPixmap(scaledPixmap)


class Document(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent=parent)
        self.setObjectName("DocumentInterface")

        # 创建布局并设置参数
        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)

        # 创建文档UI并添加到布局
        self.document_ui = DocumentUI()
        layout.addWidget(self.document_ui)

        self.document_ui.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.document_ui.setWidgetResizable(False)
        self.document_ui.setStyleSheet("background: transparent;")
        self.document_ui.setFrameShape(QFrame.NoFrame)

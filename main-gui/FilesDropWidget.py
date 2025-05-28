from pathlib import Path

from PyQt5.QtCore import Qt, pyqtSignal, QRectF
from PyQt5.QtGui import QPainter, QPainterPath, QColor, QPen, QFont
from PyQt5.QtWidgets import QWidget, QLabel

from qfluentwidgets import TeachingTip, TeachingTipTailPosition, InfoBarIcon, isDarkTheme


class FilesDropWidget(QWidget):
    pathChanged = pyqtSignal(str)

    def __init__(self, file_types, parent=None):
        super().__init__(parent)
        self.file_types = file_types
        self.target_card = None  # 用于指向 FileListSettingCard
        self._hover = False

        self.setFixedSize(280, 280)
        self.setObjectName("FilesDropWidget")

        self.text_label = QLabel(self)
        self.text_label.setText("拖  放  文  件\n\n到  此  区  域")
        self.text_label.setFont(QFont("萝莉体", 18))
        self.text_label.setAlignment(Qt.AlignCenter)
        self.text_label.setGeometry(15, 15, 250, 250)
        self.text_label.setWordWrap(True)  # 启用换行
        self.text_label.setStyleSheet("color: gray;")  # 设置文字颜色为灰色

        self.setAcceptDrops(True)
        self.setAttribute(Qt.WA_Hover)
        self.setMouseTracking(True)

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHints(QPainter.Antialiasing | QPainter.SmoothPixmapTransform)

        # 绘制背景
        path = QPainterPath()
        rect = QRectF(self.rect()).adjusted(2, 2, -2, -2)  # 内边距
        path.addRoundedRect(rect, 15, 15)  # 圆角半径

        # 绘制边框
        border_color = QColor("#3AA3FF" if self._hover else "#A0A0A0")
        if isDarkTheme():
            border_color = QColor("#3AA3FF" if self._hover else "#707070")

        pen = QPen(border_color, 3)  # 线宽
        pen.setDashPattern([4, 4])  # 4像素实线+4像素间隔
        painter.setPen(pen)
        painter.drawPath(path)

    def enterEvent(self, event):
        self._hover = True
        self.update()

    def leaveEvent(self, event):
        self._hover = False
        self.update()

    def dragEnterEvent(self, event):
        if event.mimeData().hasUrls():
            self._hover = True
            event.acceptProposedAction()
        else:
            super().dragEnterEvent(event)

    def dragMoveEvent(self, event):
        if event.mimeData().hasUrls():
            event.acceptProposedAction()

    def dropEvent(self, event):
        if event.mimeData().hasUrls():
            for url in event.mimeData().urls():
                path = url.toLocalFile()
                file_suffix = Path(path).suffix.lower()
                if file_suffix not in self.file_types:
                    self._show_error_tip("无效文件类型", f"{file_suffix} 不是支持的文件类型")
                    continue
                self.pathChanged.emit(path)  # 直接发出信号，让 FileListSettingCard 处理
            event.accept()
            self._hover = False  # 重置悬停状态
            self.update()
            return
        event.ignore()
        self._hover = False  # 重置悬停状态
        self.update()

    def bindFileListSettingCard(self, card):
        self.target_card = card
        self.pathChanged.connect(self.__update_file_list)

    def __update_file_list(self, path):
        file_suffix = Path(path).suffix.lower()
        file_type = file_suffix[1:]  # 去掉点
        self.target_card.updateFile(file_type, path)  # 通知 FileListSettingCard 更新文件

    def _show_error_tip(self, title, content):
        TeachingTip.create(
            target=self,
            icon=InfoBarIcon.ERROR,
            title=title,
            content=content,
            tailPosition=TeachingTipTailPosition.BOTTOM,
            duration=3000,
            parent=self.window()
        )

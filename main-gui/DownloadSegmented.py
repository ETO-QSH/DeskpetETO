import sys

from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import QApplication, QWidget, QStackedWidget, QVBoxLayout, QLabel

from DeskpetETO.UserCard import PreviewWindow
from qfluentwidgets import SegmentedWidget


class DownloadSegmented(QWidget):
    def __init__(self):
        super().__init__()
        self.setObjectName("DownloadInterface")
        self.setContentsMargins(35, 25, 35, 15)

        self.pivot = SegmentedWidget(self)
        self.stackedWidget = QStackedWidget(self)
        self.vBoxLayout = QVBoxLayout(self)

        self.gitInterface = PreviewWindow()
        self.userInterface = PreviewWindow()

        # add items to pivot
        self.addSubInterface(self.gitInterface, 'gitDownload', '资源仓库更新')
        self.addSubInterface(self.userInterface, 'userDownload', '用户自主提交')

        self.vBoxLayout.addWidget(self.pivot)
        self.vBoxLayout.addSpacing(10)
        self.vBoxLayout.addWidget(self.stackedWidget)

        self.stackedWidget.currentChanged.connect(self.onCurrentIndexChanged)
        self.stackedWidget.setCurrentWidget(self.gitInterface)
        self.pivot.setCurrentItem(self.gitInterface.objectName())

    def addSubInterface(self, widget: QLabel, objectName, text):
        widget.setObjectName(objectName)
        self.stackedWidget.addWidget(widget)
        self.pivot.addItem(
            routeKey=objectName,
            text=text,
            onClick=lambda: self.stackedWidget.setCurrentWidget(widget),
        )

    def onCurrentIndexChanged(self, index):
        widget = self.stackedWidget.widget(index)
        self.pivot.setCurrentItem(widget.objectName())


if __name__ == '__main__':
    QApplication.setHighDpiScaleFactorRoundingPolicy(Qt.HighDpiScaleFactorRoundingPolicy.PassThrough)
    QApplication.setAttribute(Qt.AA_EnableHighDpiScaling)
    QApplication.setAttribute(Qt.AA_UseHighDpiPixmaps)

    app = QApplication(sys.argv)
    w = DownloadSegmented()
    w.show()
    app.exec_()

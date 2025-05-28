from PyQt5.QtCore import QRectF, Qt, QPoint, QTimer, QPropertyAnimation
from PyQt5.QtGui import QPainter
from PyQt5.QtWidgets import QGraphicsOpacityEffect, QLabel, QWidget, QHBoxLayout

from qfluentwidgets import Theme, isDarkTheme, FluentStyleSheet, FluentIcon as FIF


class StateToolTip(QWidget):
    """ State tooltip """

    def __init__(self, title, parent=None):
        super().__init__(parent)
        self.title = title

        self.titleLabel = QLabel(self.title, self)
        self.rotateTimer = QTimer(self)

        self.opacityEffect = QGraphicsOpacityEffect(self)
        self.animation = QPropertyAnimation(self.opacityEffect, b"opacity")

        self.isDone = False
        self.rotateAngle = 0
        self.deltaAngle = 20

        self.__initWidget()

    def __initWidget(self):
        """ initialize widgets """
        self.setAttribute(Qt.WA_StyledBackground)
        self.setGraphicsEffect(self.opacityEffect)
        self.opacityEffect.setOpacity(1)
        self.rotateTimer.setInterval(50)

        # connect signal to slot
        self.rotateTimer.timeout.connect(self.__rotateTimerFlowSlot)

        self.__setQss()
        self.__initLayout()

        self.rotateTimer.start()

    def __initLayout(self):
        """ initialize layout """
        # 创建一个水平布局
        layout = QHBoxLayout()
        layout.setContentsMargins(20, 0, 0, 0)

        # 添加一个空的占位符，用于左对齐
        layout.addStretch()

        # 添加图标和标题到布局
        layout.addWidget(self.titleLabel)

        # 添加一个空的占位符，用于右对齐
        layout.addStretch()

        # 设置布局
        self.setLayout(layout)

    def __setQss(self):
        """ set style sheet """
        self.titleLabel.setObjectName("titleLabel")
        FluentStyleSheet.STATE_TOOL_TIP.apply(self)
        self.titleLabel.adjustSize()

    def setTitle(self, title: str):
        """ set the title of tooltip """
        self.title = title
        self.titleLabel.setText(title)
        self.titleLabel.adjustSize()

    def setState(self, isDone=False):
        """ set the state of tooltip """
        self.isDone = isDone
        self.update()
        if isDone:
            QTimer.singleShot(1000, self.__fadeOut)

    def __fadeOut(self):
        """ fade out """
        self.rotateTimer.stop()
        self.animation.setDuration(200)
        self.animation.setStartValue(1)
        self.animation.setEndValue(0)
        self.animation.finished.connect(self.deleteLater)
        self.animation.start()

    def __rotateTimerFlowSlot(self):
        """ rotate timer time out slot """
        self.rotateAngle = (self.rotateAngle + self.deltaAngle) % 360
        self.update()

    def getSuitablePos(self):
        """ get suitable position in main window """
        for i in range(10):
            dy = i*(self.height() + 16)
            pos = QPoint(self.parent().width() - self.width() - 24, 50+dy)
            widget = self.parent().childAt(pos + QPoint(2, 2))
            if isinstance(widget, StateToolTip):
                pos += QPoint(0, self.height() + 16)
            else:
                break

        return pos

    def paintEvent(self, e):
        """ paint state tooltip """
        super().paintEvent(e)
        painter = QPainter(self)
        painter.setRenderHints(QPainter.Antialiasing)
        painter.setPen(Qt.NoPen)
        theme = Theme.DARK if not isDarkTheme() else Theme.LIGHT

        if not self.isDone:
            painter.translate(19, 18)
            painter.rotate(self.rotateAngle)
            FIF.SYNC.render(painter, QRectF(-8, -8, 16, 16), theme)
        else:
            FIF.COMPLETED.render(painter, QRectF(11, 10, 16, 16), theme)


if __name__ == '__main__':
    import sys
    from PyQt5.QtWidgets import QApplication

    app = QApplication(sys.argv)
    window = StateToolTip("working...", parent=None)
    window.show()
    sys.exit(app.exec_())

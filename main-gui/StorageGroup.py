from PyQt5 import QtCore, QtWidgets
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QFont, QPixmap

from qfluentwidgets import ElevatedCardWidget, SimpleCardWidget, TitleLabel, ImageLabel


class CardGroup(object):
    def setupUi(self, Form):
        Form.setObjectName("Form")
        Form.resize(520, 690)

        self.SimpleCardWidget = SimpleCardWidget(Form)
        self.SimpleCardWidget.setGeometry(QtCore.QRect(0, 0, 520, 690))
        self.SimpleCardWidget.setMinimumSize(QtCore.QSize(520, 690))
        self.SimpleCardWidget.setMaximumSize(QtCore.QSize(520, 690))
        self.SimpleCardWidget.setObjectName("SimpleCardWidget")

        self.gridLayout = QtWidgets.QGridLayout(self.SimpleCardWidget)
        self.gridLayout.setContentsMargins(5, 5, 5, 5)
        self.gridLayout.setSpacing(5)  # 设置控件之间的间隔
        self.gridLayout.setObjectName("gridLayout")

        for n in range(1, 13):
            elevated_card = ElevatedCardWidget(self.SimpleCardWidget)
            elevated_card.setMinimumSize(QtCore.QSize(160, 160))
            elevated_card.setMaximumSize(QtCore.QSize(160, 160))
            elevated_card.setObjectName(f"ElevatedCardWidget_{n}")

            self.iconBadge = ImageLabel()
            self.iconBadge.setPixmap(QPixmap())
            self.iconBadge.setFixedSize(150, 150)
            self.iconBadge.setAlignment(Qt.AlignCenter)
            self.iconBadge.setBorderRadius(8, 8, 8, 8)

            elevated_card.setLayout(QtWidgets.QVBoxLayout())
            elevated_card.layout().setContentsMargins(0, 0, 0, 0)
            elevated_card.layout().setAlignment(QtCore.Qt.AlignmentFlag.AlignCenter)
            elevated_card.layout().addWidget(self.iconBadge)

            self.gridLayout.addWidget(elevated_card, (n-1)//3, (n-1)%3, 1, 1)

        QtCore.QMetaObject.connectSlotsByName(Form)


class StorageGroup(QtWidgets.QWidget):
    def __init__(self):
        super().__init__()
        self.ui = CardGroup()
        self.ui.setupUi(self)

        self.main_layout = QtWidgets.QHBoxLayout(self)
        self.main_layout.setContentsMargins(0, 0, 0, 0)
        self.main_layout.setSpacing(0)

        self.vertical_layout = QtWidgets.QVBoxLayout()
        self.vertical_layout.setContentsMargins(0, 0, 0, 0)
        self.vertical_layout.setSpacing(0)

        self.vertical_layout.addStretch(1)

        self.titleLabel = TitleLabel("桌  宠  收  纳  工  具")
        self.titleLabel.setFont(QFont("萝莉体", 20))
        self.titleLabel.setAlignment(QtCore.Qt.AlignCenter)
        self.vertical_layout.addWidget(self.titleLabel)

        self.vertical_layout.addStretch(1)
        self.vertical_layout.addWidget(self.ui.SimpleCardWidget)
        self.vertical_layout.addStretch(1)

        self.main_layout.addStretch(1)
        self.main_layout.addLayout(self.vertical_layout)
        self.main_layout.addStretch(1)

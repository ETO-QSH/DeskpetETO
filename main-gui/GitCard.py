from PyQt5.QtWidgets import QVBoxLayout, QHBoxLayout

from qfluentwidgets import SimpleCardWidget

from DeskpetETO.VersionUpdateCard import VersionUpdateCard, sample_data, current_tag


class GitDownloadInterface(SimpleCardWidget):
    """ 包含两个版本更新卡片的卡片容器 """

    def __init__(self, parent=None):
        super().__init__(parent)
        # 设置固定尺寸
        self.setFixedSize(450, 720)

        # 主水平布局
        main_layout = QHBoxLayout(self)

        # 左侧弹簧
        main_layout.addStretch(1)

        # 中央垂直布局
        center_layout = QVBoxLayout()

        # 顶部弹簧
        center_layout.addStretch(1)

        # 创建并添加卡片
        self.card1 = VersionUpdateCard(titel="模型更新", version_data=sample_data, current_tag=current_tag)
        center_layout.addWidget(self.card1)

        # 中间弹簧
        center_layout.addStretch(1)

        self.card2 = VersionUpdateCard(titel="软件更新", version_data=sample_data, current_tag=current_tag)
        center_layout.addWidget(self.card2)

        # 底部弹簧
        center_layout.addStretch(1)

        main_layout.addLayout(center_layout)

        # 右侧弹簧
        main_layout.addStretch(1)

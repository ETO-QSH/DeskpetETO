from PyQt5.QtWidgets import QVBoxLayout, QHBoxLayout

from qfluentwidgets import SimpleCardWidget

from DeskpetETO.Setting import cfg
from DeskpetETO.VersionUpdateCard import VersionUpdateCard


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
        self.card1 = VersionUpdateCard(title="模型更新", current_info=cfg.get(cfg.library_update), owner_repo=("ETO-QSH", "DeskpetETO-download"))
        center_layout.addWidget(self.card1)

        # 中间弹簧
        center_layout.addStretch(1)

        self.card2 = VersionUpdateCard(title="软件更新", current_info=cfg.get(cfg.software_update), owner_repo=("ETO-QSH", "DeskpetETO-download"))
        center_layout.addWidget(self.card2)

        # 底部弹簧
        center_layout.addStretch(1)

        main_layout.addLayout(center_layout)

        # 右侧弹簧
        main_layout.addStretch(1)

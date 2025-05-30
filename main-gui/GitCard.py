from PyQt5.QtWidgets import QVBoxLayout, QHBoxLayout

from DeskpetETO.test import VersionUpdateCard, sample_data, current_tag
from qfluentwidgets import SimpleCardWidget


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
        self.card1 = VersionUpdateCard(version_data=sample_data, current_tag=current_tag)
        center_layout.addWidget(self.card1)

        # 中间弹簧
        center_layout.addStretch(1)

        self.card2 = VersionUpdateCard(version_data=sample_data, current_tag=current_tag)
        center_layout.addWidget(self.card2)

        # 底部弹簧
        center_layout.addStretch(1)

        main_layout.addLayout(center_layout)

        # 右侧弹簧
        main_layout.addStretch(1)

        # 自定义卡片数据（示例）
        self.card1.current_version = "v1.0.0"
        self.card2.current_version = "v2.1.3"
        self.card1.version_data = {
            "v1.0.0": ["• 初始版本"],
            "v1.1.0": ["• 新增功能A", "• 修复问题B"]
        }
        self.card2.version_data = {
            "v2.1.3": ["• 当前稳定版"],
            "v2.2.0": ["• 新增模块X", "• 优化性能Y"]
        }

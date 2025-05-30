from PyQt5.QtCore import Qt
from PyQt5.QtGui import QFont
from PyQt5.QtWidgets import QVBoxLayout, QHBoxLayout, QWidget, QStackedWidget, QFrame

from qfluentwidgets import (
    HeaderCardWidget, BodyLabel, StrongBodyLabel, PrimaryPushButton, FluentIcon, SimpleCardWidget,
    SmoothScrollArea, TransparentToolButton, ComboBox
)


class VersionUpdateCard(HeaderCardWidget):
    def __init__(self, titel, version_data, current_tag, parent=None):
        super().__init__(parent)
        self.setFixedSize(420, 335)
        self.setTitle(titel)  # 固定标题

        # 处理版本数据
        self.version_data = self._process_data(version_data)
        self.current_tag = current_tag.replace('-', '.')

        self.viewLayout.setContentsMargins(24, 0, 24, 0)

        # 初始化UI
        self._init_ui()
        self._refresh_info(self.current_tag)

    def _init_ui(self):
        # 版本选择部件
        self.version_stack = QStackedWidget()
        self._init_version_widgets()

        # 创建内容卡片
        self.content_card = SimpleCardWidget()
        self.content_card.setObjectName("contentCard")
        self._init_content_card()

        # 操作按钮
        self.updateButton = PrimaryPushButton("获 取 更 新 信 息", self)
        self.updateButton.setFont(QFont('萝莉体', 12))
        self.updateButton.clicked.connect(self._handle_click)

        # 主布局
        main_layout = QVBoxLayout()
        main_layout.addStretch(1)
        main_layout.addWidget(self.version_stack)
        main_layout.addStretch(1)
        main_layout.addWidget(self.content_card)
        main_layout.addStretch(1)
        main_layout.addWidget(self.updateButton)
        main_layout.addStretch(2)

        self.viewLayout.addLayout(main_layout)

    def _init_version_widgets(self):
        """初始化版本显示组件"""
        # 当前版本状态
        current_widget = QWidget()
        current_layout = QHBoxLayout(current_widget)
        current_layout.setContentsMargins(0, 0, 0, 0)
        current_layout.addWidget(StrongBodyLabel("当前版本："))
        self.current_label = StrongBodyLabel(self.current_tag)
        current_layout.addWidget(self.current_label)
        current_layout.addStretch(1)

        # 目标版本状态
        target_widget = QWidget()
        target_layout = QHBoxLayout(target_widget)
        target_layout.setContentsMargins(0, 0, 0, 0)
        target_layout.addWidget(StrongBodyLabel("目标版本："))
        target_layout.addWidget(StrongBodyLabel(self.current_tag))
        target_layout.addWidget(TransparentToolButton(FluentIcon.PAGE_RIGHT))

        self.version_combo = ComboBox()

        target_layout.addWidget(self.version_combo)
        target_layout.addStretch(1)

        self.version_stack.addWidget(current_widget)
        self.version_stack.addWidget(target_widget)

    def _init_content_card(self):
        """初始化内容卡片"""
        card_layout = QVBoxLayout(self.content_card)
        self.scrollWidget = QWidget()
        self.detailLayout = QVBoxLayout(self.scrollWidget)

        # 滚动区域
        self.scrollArea = SmoothScrollArea()
        self.scrollArea.setObjectName("contentScrollArea")
        self.scrollArea.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.scrollArea.setWidgetResizable(False)
        self.scrollArea.setStyleSheet("background: transparent;")
        self.scrollArea.setFrameShape(QFrame.NoFrame)

        self.scrollArea.setWidget(self.scrollWidget)
        self.scrollArea.setWidgetResizable(True)
        self.scrollArea.setMinimumHeight(150)

        card_layout.addWidget(self.scrollArea)

    def _process_data(self, raw_data):
        """处理原始数据格式"""
        processed = {}
        for item in raw_data:
            # 统一版本号格式
            tag = item['tag'].replace('-', '.')
            processed[tag] = {
                'title': item['title'],
                'items': [f"• {line}" for line in item['body'].split('\n') if line.strip()],
                'url': item['assets'][0]['download_url'] if item['assets'] else None
            }
        return processed

    def _parse_version(self, tag):
        """解析版本号为可比较的元组"""
        return tuple(map(int, tag.split('.')))

    def _refresh_info(self, tag):
        """刷新显示内容"""
        # 清空现有内容
        while self.detailLayout.count():
            child = self.detailLayout.takeAt(0)
            if child.widget():
                child.widget().deleteLater()

        # 添加动态标题
        title_label = StrongBodyLabel(self.version_data[tag]['title'])
        title_label.setAlignment(Qt.AlignCenter)
        self.detailLayout.addWidget(title_label)
        self.detailLayout.addSpacing(5)

        # 添加内容项
        for item in self.version_data[tag]['items']:
            self.detailLayout.addWidget(BodyLabel(item))

        self.detailLayout.addStretch(1)

    def _get_available_versions(self):
        """获取可用版本列表"""
        return sorted(
            [tag for tag in self.version_data.keys()
             if self._parse_version(tag) >= self._parse_version(self.current_tag)],
            key=self._parse_version,
            reverse=True
        )

    def _switch_mode(self):
        """切换到版本选择模式"""
        self.version_stack.setCurrentIndex(1)
        self.version_combo.clear()

        versions = self._get_available_versions()
        self.version_combo.addItems(versions)

        version_count = len(versions)
        self.version_combo.setMaxVisibleItems(version_count if version_count <= 5 else 5)

        if versions:
            self.version_combo.setCurrentIndex(0)
            self._refresh_info(versions[0])

        self.version_combo.currentTextChanged.connect(self._refresh_info)
        self.updateButton.setText("下 载 指 定 版 本")

    def _handle_click(self):
        """按钮点击处理"""
        if self.version_stack.currentIndex() == 0:
            self._switch_mode()
        else:
            selected = self.version_combo.currentText()
            url = self.version_data[selected]['url']
            print(f"开始下载：{url}")



sample_data = [{'tag': '2025.5.2', 'title': 'Deskpet model before 2025-5-2', 'body': '截止至【众生行记】\n截止至【众生行记】\n截止至【众生行记】\n截止至【众生行记】\n截止至【众生行记】\n截止至【众生行记】\n截止至【众生行记】\n截止至【众生行记】\n截止至【众生行记】\n截止至【众生行记】\n截止至【众生行记】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2025.5.2/DeskpetETO.zip'}]}, {'tag': '2025.4.22', 'title': 'Deskpet model before 2025-4-22', 'body': '截止至【巴别塔·复刻】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2025.4.22/DeskpetETO.zip'}]}, {'tag': '2024.10.10', 'title': 'Deskpet model before 2024-10-10', 'body': '截止至【追迹日落以西】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.10.10/DeskpetETO.zip'}]}, {'tag': '2024.9.28', 'title': 'Deskpet model before 2024-9-28', 'body': '截止至【矢量突破】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.9.28/DeskpetETO.zip'}]}, {'tag': '2024.9.20', 'title': 'Deskpet model before 2024-9-20', 'body': '截止至【不义之财·复刻】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.9.20/DeskpetETO.zip'}]}, {'tag': '2024.9.16', 'title': 'Deskpet model before 2024-9-16', 'body': '截止至【良辰迎月】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.9.16/DeskpetETO.zip'}]}, {'tag': '2024.9.2', 'title': 'Deskpet model before 2024-9-2', 'body': '截止至【泰拉饭】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.9.2/DeskpetETO.zip'}]}, {'tag': '2024.8.22', 'title': 'Deskpet model before 2024-8-22', 'body': '截止至【火山旅梦·复刻】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.8.22/DeskpetETO.zip'}]}, {'tag': '2024.8.9', 'title': 'Deskpet model before 2024-8-9', 'body': '截止至【沉沙赫日】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.8.9/DeskpetETO.zip'}]}, {'tag': '2024.8.3', 'title': 'Deskpet model before 2024-8-3', 'body': '截止至【太阳甩在身后】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.8.3/DeskpetETO.zip'}]}, {'tag': '2024-7-21', 'title': 'Deskpet model before 2024-7-21', 'body': '截止至【萨卡兹的无终奇语】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024-7-21/DeskpetETO.zip'}]}, {'tag': '2024.7.10', 'title': 'Deskpet model before 2024-7-10', 'body': '截止至【熔炉“还魂”记】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.7.10/DeskpetETO.zip'}]}, {'tag': '2024.6.28', 'title': 'Deskpet model before 2024-6-28', 'body': '截止至【空想花庭·复刻】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.6.28/DeskpetETO.zip'}]}, {'tag': '2024.6.20', 'title': 'Deskpet model before 2024-6-20', 'body': '截止至【虹彩茶会】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.6.20/DeskpetETO.zip'}]}, {'tag': '2024.6.6', 'title': 'Deskpet model before 2024-6-6', 'body': '截止至【生路】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.6.6/DeskpetETO.zip'}]}, {'tag': '2024.5.27', 'title': 'Deskpet model before 2024-5-27', 'body': '截止至【「沙洲遗闻」6月月度更新】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.5.27/DeskpetETO.zip'}]}, {'tag': '2024.5.17', 'title': 'Deskpet model before 2024-5-17', 'body': '截止至【促融共竞】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.5.17/DeskpetETO.zip'}]}, {'tag': '2024.5.3', 'title': 'Deskpet model before 2024-5-3', 'body': '截止至【慈悲灯塔】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.5.3/DeskpetETO.zip'}]}, {'tag': '2024.4.26', 'title': 'Deskpet model before 2024-4-26', 'body': '截止至【音律联觉纪念2024】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.4.26/DeskpetETO.zip'}]}, {'tag': '2024.4.11', 'title': 'Deskpet model before 2024-4-11', 'body': '截止至【巴别塔】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.4.11/DeskpetETO.zip'}]}, {'tag': '2024.2.15', 'title': 'Deskpet model before 2024-2-15', 'body': '截止至【沙洲遗闻】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.2.15/DeskpetETO.zip'}]}, {'tag': '2024.1.20', 'title': 'Deskpet model before 2024-1-20', 'body': '截止至【登临意·复刻】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.1.20/DeskpetETO.zip'}]}, {'tag': '2024.1.11', 'title': 'Deskpet model before 2024-1-11', 'body': '截止至【去咧嘴谷】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.1.11/DeskpetETO.zip'}]}, {'tag': '2023.12.24', 'title': 'Deskpet model before 2023-12-24', 'body': '截止至【跨年纪念2024】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2023.12.24/DeskpetETO.zip'}]}, {'tag': '2023.11.12', 'title': 'Deskpet model before 2023-11-12', 'body': '截止至【崔林特尔梅之金】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2023.11.12/DeskpetETO.zip'}]}, {'tag': '2023.10.11', 'title': 'Deskpet model before 2023-10-11', 'body': '截止至【恶兆湍流】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2023.10.11/DeskpetETO.zip'}]}, {'tag': '2023.10.5', 'title': 'Deskpet model before 2023-10-5', 'body': '截止至【纷争演绎】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2023.10.5/DeskpetETO.zip'}]}]
current_tag = "2024.6.20"


if __name__ == "__main__":
    from PyQt5.QtWidgets import QApplication
    import sys

    # 示例数据（需替换为实际数据）

    app = QApplication(sys.argv)
    card = VersionUpdateCard(
        titel="示例结构",
        version_data=sample_data,
        current_tag=current_tag
    )
    card.show()
    sys.exit(app.exec_())

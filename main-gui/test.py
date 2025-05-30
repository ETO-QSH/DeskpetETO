from qfluentwidgets import (
    SimpleCardWidget, TitleLabel, StrongBodyLabel, BodyLabel,
    SingleDirectionScrollArea, PrimaryPushButton, ComboBox,
    IconWidget, FluentIcon as FIF, SubtitleLabel, HeaderCardWidget
)
from PyQt5.QtWidgets import QVBoxLayout, QHBoxLayout, QWidget, QStackedWidget
from PyQt5.QtCore import Qt


class VersionUpdateCard(HeaderCardWidget):
    def __init__(self, version_data, current_tag, parent=None):
        super().__init__(parent)
        self.setFixedSize(520, 420)
        self.version_data = self._process_data(version_data)
        self.current_tag = current_tag.replace("-", ".")
        self.setTitle("版本更新")

        # 初始化UI
        self._init_ui()
        self._show_current_version()

    def _init_ui(self):
        # 主布局
        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(20, 15, 20, 15)
        main_layout.setSpacing(12)

        # 版本选择部件
        self._init_version_selector()
        main_layout.addWidget(self.version_stack)

        # 滚动内容区域
        self._init_scroll_area()
        main_layout.addWidget(self.scrollArea)

        # 操作按钮
        self.actionButton = PrimaryPushButton("获取最新版本", self)
        self.actionButton.clicked.connect(self._handle_action)
        main_layout.addWidget(self.actionButton)

    def _init_version_selector(self):
        """版本选择组件"""
        self.version_stack = QStackedWidget()

        # 初始状态（当前版本）
        current_widget = QWidget()
        current_layout = QHBoxLayout(current_widget)
        current_layout.setContentsMargins(0, 0, 0, 0)
        current_layout.addWidget(StrongBodyLabel("当前版本："))
        self.current_label = StrongBodyLabel(self.current_tag)
        current_layout.addWidget(self.current_label)
        current_layout.addStretch(1)
        self.version_stack.addWidget(current_widget)

        # 目标版本选择状态
        target_widget = QWidget()
        target_layout = QHBoxLayout(target_widget)
        target_layout.setContentsMargins(0, 0, 0, 0)
        target_layout.addWidget(StrongBodyLabel("目标版本："))
        target_layout.addWidget(StrongBodyLabel(self.current_tag))
        target_layout.addWidget(IconWidget(FIF.CHEVRON_RIGHT))

        self.version_combo = ComboBox()
        target_layout.addWidget(self.version_combo)
        target_layout.addStretch(1)
        self.version_stack.addWidget(target_widget)

    def _init_scroll_area(self):
        """初始化带标题的滚动区域"""
        self.scrollArea = SingleDirectionScrollArea(orient=Qt.Vertical)
        self.scrollWidget = QWidget()
        scroll_layout = QVBoxLayout(self.scrollWidget)

        # 滚动区域内的居中标题
        self.contentTitleLabel = StrongBodyLabel()
        self.contentTitleLabel.setAlignment(Qt.AlignCenter)
        self.contentTitleLabel.setStyleSheet("font-size: 14px; margin: 8px 0;")
        scroll_layout.addWidget(self.contentTitleLabel)

        # 内容条目
        self.contentLayout = QVBoxLayout()
        self.contentLayout.setSpacing(6)
        scroll_layout.addLayout(self.contentLayout)
        scroll_layout.addStretch(1)

        self.scrollArea.setWidget(self.scrollWidget)
        self.scrollArea.setWidgetResizable(True)
        self.scrollArea.setFixedHeight(200)

    def _process_data(self, raw_data):
        """处理原始数据为内部格式"""
        processed = {}
        for item in raw_data:
            tag = item['tag'].replace("-", ".")
            processed[tag] = {
                'title': item['title'],
                'body': [f"• {line}" for line in item['body'].split("\n") if line.strip()],
                'url': item['assets'][0]['download_url'] if item['assets'] else None
            }
        return processed

    def _parse_version(self, tag):
        """将版本号转换为可比较的元组"""
        return tuple(map(int, tag.split(".")))

    def _show_content(self, tag):
        """显示指定版本内容"""
        data = self.version_data.get(tag, {})

        # 更新滚动区域标题
        self.contentTitleLabel.setText(data.get('title', '未知版本'))

        # 清空旧内容
        while self.contentLayout.count():
            item = self.contentLayout.takeAt(0)
            if item.widget():
                item.widget().deleteLater()

        # 添加新内容
        for line in data.get('body', ["• 暂无更新信息"]):
            self.contentLayout.addWidget(BodyLabel(line))

    def _show_current_version(self):
        """显示当前版本内容"""
        self._show_content(self.current_tag)

    def _get_available_versions(self):
        """获取可用版本列表（降序）"""
        return sorted(
            [tag for tag in self.version_data.keys()
             if self._parse_version(tag) >= self._parse_version(self.current_tag)],
            key=self._parse_version,
            reverse=True
        )

    def _switch_to_target_mode(self):
        """切换到版本选择模式"""
        self.version_stack.setCurrentIndex(1)

        # 填充版本数据
        self.version_combo.clear()
        versions = self._get_available_versions()
        self.version_combo.addItems(versions)

        if versions:
            self.version_combo.setCurrentText(versions[0])
            self._show_content(versions[0])

        self.version_combo.currentTextChanged.connect(self._show_content)
        self.actionButton.setText("下载指定版本")

    def _handle_action(self):
        """处理按钮操作"""
        if self.version_stack.currentIndex() == 0:
            self._switch_to_target_mode()
        else:
            selected_tag = self.version_combo.currentText()
            download_url = self.version_data[selected_tag]['url']
            print(f"开始下载：{download_url}")


sample_data = [{'tag': '2025.5.2', 'title': 'Deskpet model before 2025-5-2', 'body': '截止至【众生行记】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2025.5.2/DeskpetETO.zip'}]}, {'tag': '2025.4.22', 'title': 'Deskpet model before 2025-4-22', 'body': '截止至【巴别塔·复刻】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2025.4.22/DeskpetETO.zip'}]}, {'tag': '2024.10.10', 'title': 'Deskpet model before 2024-10-10', 'body': '截止至【追迹日落以西】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.10.10/DeskpetETO.zip'}]}, {'tag': '2024.9.28', 'title': 'Deskpet model before 2024-9-28', 'body': '截止至【矢量突破】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.9.28/DeskpetETO.zip'}]}, {'tag': '2024.9.20', 'title': 'Deskpet model before 2024-9-20', 'body': '截止至【不义之财·复刻】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.9.20/DeskpetETO.zip'}]}, {'tag': '2024.9.16', 'title': 'Deskpet model before 2024-9-16', 'body': '截止至【良辰迎月】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.9.16/DeskpetETO.zip'}]}, {'tag': '2024.9.2', 'title': 'Deskpet model before 2024-9-2', 'body': '截止至【泰拉饭】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.9.2/DeskpetETO.zip'}]}, {'tag': '2024.8.22', 'title': 'Deskpet model before 2024-8-22', 'body': '截止至【火山旅梦·复刻】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.8.22/DeskpetETO.zip'}]}, {'tag': '2024.8.9', 'title': 'Deskpet model before 2024-8-9', 'body': '截止至【沉沙赫日】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.8.9/DeskpetETO.zip'}]}, {'tag': '2024.8.3', 'title': 'Deskpet model before 2024-8-3', 'body': '截止至【太阳甩在身后】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.8.3/DeskpetETO.zip'}]}, {'tag': '2024-7-21', 'title': 'Deskpet model before 2024-7-21', 'body': '截止至【萨卡兹的无终奇语】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024-7-21/DeskpetETO.zip'}]}, {'tag': '2024.7.10', 'title': 'Deskpet model before 2024-7-10', 'body': '截止至【熔炉“还魂”记】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.7.10/DeskpetETO.zip'}]}, {'tag': '2024.6.28', 'title': 'Deskpet model before 2024-6-28', 'body': '截止至【空想花庭·复刻】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.6.28/DeskpetETO.zip'}]}, {'tag': '2024.6.20', 'title': 'Deskpet model before 2024-6-20', 'body': '截止至【虹彩茶会】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.6.20/DeskpetETO.zip'}]}, {'tag': '2024.6.6', 'title': 'Deskpet model before 2024-6-6', 'body': '截止至【生路】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.6.6/DeskpetETO.zip'}]}, {'tag': '2024.5.27', 'title': 'Deskpet model before 2024-5-27', 'body': '截止至【「沙洲遗闻」6月月度更新】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.5.27/DeskpetETO.zip'}]}, {'tag': '2024.5.17', 'title': 'Deskpet model before 2024-5-17', 'body': '截止至【促融共竞】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.5.17/DeskpetETO.zip'}]}, {'tag': '2024.5.3', 'title': 'Deskpet model before 2024-5-3', 'body': '截止至【慈悲灯塔】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.5.3/DeskpetETO.zip'}]}, {'tag': '2024.4.26', 'title': 'Deskpet model before 2024-4-26', 'body': '截止至【音律联觉纪念2024】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.4.26/DeskpetETO.zip'}]}, {'tag': '2024.4.11', 'title': 'Deskpet model before 2024-4-11', 'body': '截止至【巴别塔】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.4.11/DeskpetETO.zip'}]}, {'tag': '2024.2.15', 'title': 'Deskpet model before 2024-2-15', 'body': '截止至【沙洲遗闻】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.2.15/DeskpetETO.zip'}]}, {'tag': '2024.1.20', 'title': 'Deskpet model before 2024-1-20', 'body': '截止至【登临意·复刻】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.1.20/DeskpetETO.zip'}]}, {'tag': '2024.1.11', 'title': 'Deskpet model before 2024-1-11', 'body': '截止至【去咧嘴谷】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2024.1.11/DeskpetETO.zip'}]}, {'tag': '2023.12.24', 'title': 'Deskpet model before 2023-12-24', 'body': '截止至【跨年纪念2024】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2023.12.24/DeskpetETO.zip'}]}, {'tag': '2023.11.12', 'title': 'Deskpet model before 2023-11-12', 'body': '截止至【崔林特尔梅之金】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2023.11.12/DeskpetETO.zip'}]}, {'tag': '2023.10.11', 'title': 'Deskpet model before 2023-10-11', 'body': '截止至【恶兆湍流】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2023.10.11/DeskpetETO.zip'}]}, {'tag': '2023.10.5', 'title': 'Deskpet model before 2023-10-5', 'body': '截止至【纷争演绎】', 'assets': [{'name': 'DeskpetETO.zip', 'download_url': 'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2023.10.5/DeskpetETO.zip'}]}]
current_tag = "2024.6.20"


if __name__ == "__main__":
    from PyQt5.QtWidgets import QApplication
    import sys

    # 示例数据（需替换为实际数据）

    app = QApplication(sys.argv)
    card = VersionUpdateCard(
        version_data=sample_data,
        current_tag=current_tag
    )
    card.show()
    sys.exit(app.exec_())

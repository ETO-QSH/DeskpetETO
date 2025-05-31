from PyQt5.QtCore import Qt, QThread
from PyQt5.QtGui import QFont
from PyQt5.QtWidgets import QVBoxLayout, QHBoxLayout, QWidget, QStackedWidget, QFrame

from qfluentwidgets import (
    HeaderCardWidget, BodyLabel, StrongBodyLabel, PrimaryPushButton, FluentIcon, SimpleCardWidget,
    SmoothScrollArea, TransparentToolButton, ComboBox, IndeterminateProgressBar, InfoBar, InfoBarPosition
)

from DeskpetETO.GithubRequest import GithubWorker
from DeskpetETO.DownloadUnzipBox import show_download_dialog


class VersionUpdateCard(HeaderCardWidget):
    def __init__(self, title, current_info, owner_repo, parent=None):
        super().__init__(parent)
        self.setFixedSize(420, 335)
        self.setTitle(title)

        # 处理当前版本信息
        self.current_info = current_info
        self.version_data = {}
        self.current_tag = current_info['tag']
        self.owner, self.repo = owner_repo

        self.viewLayout.setContentsMargins(24, 0, 24, 0)

        # 初始化UI
        self._init_ui()
        self._refresh_info()

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
        self.current_label.setFixedWidth(75)
        current_layout.addWidget(self.current_label)

        self.loading_progress = IndeterminateProgressBar()
        self.loading_progress.setFixedSize(200, 5)
        self.loading_progress.hide()
        current_layout.addWidget(self.loading_progress)

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
            processed[item['tag']] = {
                'title': item['title'], 'items': [f"• {line}" for line in item['body'].split('\n') if line.strip()],
                'url': item['assets'][0]['download_url'] if item['assets'] else None,
                'name': item['assets'][0]['name'] if item['assets'] else None,
                'size': item['assets'][0]['size'] if item['assets'] else None
            }
        return processed

    def _parse_version(self, tag):
        """解析版本号为可比较的元组"""
        return tuple(map(int, tag.split('.')))

    def _refresh_info(self):
        """刷新显示内容"""
        # 清空现有内容
        while self.detailLayout.count():
            child = self.detailLayout.takeAt(0)
            if child.widget():
                child.widget().deleteLater()

        # 添加动态标题
        title_label = StrongBodyLabel(self.current_info['title'])
        title_label.setAlignment(Qt.AlignCenter)
        self.detailLayout.addWidget(title_label)
        self.detailLayout.addSpacing(5)

        # 添加内容项
        for line in self.current_info['body'].split('\n'):
            if line.strip():
                self.detailLayout.addWidget(BodyLabel(f"• {line}"))

        self.detailLayout.addStretch(1)

    def _get_available_versions(self):
        """获取可用版本列表"""
        return sorted(
            [tag for tag in self.version_data.keys() if self._parse_version(tag) >=
             self._parse_version(self.current_tag)], key=self._parse_version, reverse=True
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
            self._update_content(versions[0])

        self.version_combo.currentTextChanged.connect(self._update_content)
        self.updateButton.setText("下 载 指 定 版 本")

    def _update_content(self, tag):
        """更新内容显示"""
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

    def _handle_click(self):
        """按钮点击处理"""
        if self.version_stack.currentIndex() == 0:
            self._fetch_versions()
        else:
            selected = self.version_combo.currentText()
            url = self.version_data[selected]['url']
            size = self.version_data[selected]['size']
            name = self.version_data[selected]['name']
            show_download_dialog(self, url, size, name)

    def _fetch_versions(self):
        """获取版本信息"""
        self.loading_progress.show()
        self.thread = QThread()
        self.worker = GithubWorker(self.owner, self.repo)
        self.worker.moveToThread(self.thread)
        self.thread.started.connect(self.worker.run)
        self.worker.finished.connect(self._on_fetch_finished)
        self.worker.finished.connect(self.thread.quit)
        self.worker.finished.connect(self.worker.deleteLater)
        self.thread.finished.connect(self.thread.deleteLater)
        self.thread.start()

    def _on_fetch_finished(self, success, data=None, error_info=None):
        """处理获取到的版本信息"""
        self.loading_progress.hide()
        if success:
            self.version_data = self._process_data(data)
            self._switch_mode()
        else:
            self._show_error_info(error_info)

    def _show_error_info(self, error_info):
        InfoBar.error(
            title="获取版本信息失败",
            content=error_info,
            orient=Qt.Horizontal,
            isClosable=True,
            position=InfoBarPosition.TOP,
            duration=3000,
            parent=self
        )


current_info = {'tag': '2023.10.5', 'title': 'Deskpet model before 2023-10-5', 'body': '截止至【纷争演绎】',
                'assets': [{'name': 'DeskpetETO.zip', 'size': 106742365, 'download_url':
                'https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2023.10.5/DeskpetETO.zip'}]}


if __name__ == "__main__":
    from PyQt5.QtWidgets import QApplication
    import sys

    app = QApplication(sys.argv)
    card = VersionUpdateCard(title="示例结构", current_info=current_info, owner_repo=("ETO-QSH", "DeskpetETO-download"))
    card.show()
    sys.exit(app.exec_())

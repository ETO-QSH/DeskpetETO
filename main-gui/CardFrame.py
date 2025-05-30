import sys
import copy
from uuid import uuid4

from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtGui import QFontDatabase, QFont

from DeskpetETO.JsonMerger import JsonMerger
from qfluentwidgets import SmoothScrollArea
from DeskpetETO.CustomCard import CustomCard, AddCard, FilterCard


class CardManager(QtCore.QObject):
    card_modified = QtCore.pyqtSignal(str)
    card_removed = QtCore.pyqtSignal(str)
    card_added = QtCore.pyqtSignal(str)

    def __init__(self):
        super().__init__()
        self.cards = {}  # {card_id: {widget: CustomCard, data: dict}}
        self.card_order = []

    def _update_indices(self, start=0):
        """更新卡片索引"""
        for i, card_id in enumerate(self.card_order[start:], start=start):
            self.cards[card_id]['data']['index'] = i


class Ui_CardFrame(object):
    def setupUi(self, CardFrame):
        CardFrame.resize(540, 810)
        CardFrame.setObjectName("CardInterface")
        CardFrame.setMinimumSize(QtCore.QSize(540, 810))
        CardFrame.setMaximumSize(QtCore.QSize(540, 810))

        # 加载自定义字体
        self.font_id = QFontDatabase.addApplicationFont(r".\resource\font\Lolita.ttf")
        if self.font_id != -1:
            self.font_family = QFontDatabase.applicationFontFamilies(self.font_id)[0]
            font = QFont(self.font_family)
            CardFrame.setFont(font)
        else:
            print("Failed to load font")

        self.widget = QtWidgets.QWidget(CardFrame)
        self.widget.setGeometry(QtCore.QRect(0, 0, 540, 810))
        self.widget.setObjectName("widget")
        self.verticalLayout = QtWidgets.QVBoxLayout(self.widget)
        self.verticalLayout.setContentsMargins(0, 0, 0, 0)
        self.verticalLayout.setObjectName("verticalLayout")

        # 初始化滚动区域
        self.SmoothScrollArea = SmoothScrollArea(self.widget)
        self.SmoothScrollArea.setGeometry(QtCore.QRect(0, 0, 540, 670))
        self.SmoothScrollArea.setMinimumSize(QtCore.QSize(540, 670))
        self.SmoothScrollArea.setMaximumSize(QtCore.QSize(540, 4500))
        self.SmoothScrollArea.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        self.SmoothScrollArea.setWidgetResizable(False)
        self.SmoothScrollArea.setStyleSheet("background: transparent;")
        self.SmoothScrollArea.setFrameShape(QtWidgets.QFrame.NoFrame)

        self.scrollContent = QtWidgets.QWidget()
        self.scrollContent.setMinimumSize(QtCore.QSize(540, 0))
        self.mainLayout = QtWidgets.QVBoxLayout(self.scrollContent)
        self.mainLayout.setContentsMargins(6, 10, 15, 9)
        self.mainLayout.setSpacing(10)

        self.SmoothScrollArea.setWidget(self.scrollContent)
        self.verticalLayout.addWidget(self.SmoothScrollArea)
        self.card_manager = CardManager()

        self.current_card_widget = None
        self.SHOW_CARD = "ADD"
        self.show_add_card()

        # 拖拽相关初始化
        self.dragging_card = None
        self.drop_line = QtWidgets.QFrame()
        self.drop_line.setFrameShape(QtWidgets.QFrame.HLine)
        self.drop_line.hide()

        # 加载一些乱七八糟的数据
        self.json_merger = JsonMerger()
        self.load_brands_data()

    def show_add_card(self):
        """显示添加卡片组件"""
        if self.current_card_widget:
            self._remove_current_card()

        self.AddCardWidget = AddCard(self.widget)
        self.AddCardWidget.setObjectName("AddCardWidget")
        self.AddCardWidget.setMinimumSize(QtCore.QSize(540, 120))
        self.AddCardWidget.setMaximumSize(QtCore.QSize(540, 120))
        self.AddCardWidget.setFont(QFont(self.font_family))
        self.AddCardWidget.setStyleSheet(f"""
            * {{
                font-family: '{self.font_family}';
            }}
        """)

        # 重置筛选显示所有卡片
        for card_id in self.card_manager.card_order:
            self.card_manager.cards[card_id]['widget'].show()

        # 连接信号
        self.AddCardWidget.switchToFilter.connect(self.show_filter_card)
        self.AddCardWidget.cardRequested.connect(self.handle_add_card_request)
        self.verticalLayout.addWidget(self.AddCardWidget, 0, QtCore.Qt.AlignCenter)
        self.current_card_widget = self.AddCardWidget

        self.SHOW_CARD = "ADD"
        self._update_all_card_icons()  # 新增状态更新
        self._reorder_layout()

    def show_filter_card(self):
        """显示筛选卡片组件"""
        if self.current_card_widget:
            self._remove_current_card()

        self.FilterCardWidget = FilterCard(self.widget)
        self.FilterCardWidget.setObjectName("FilterCardWidget")
        self.FilterCardWidget.setMinimumSize(QtCore.QSize(540, 120))
        self.FilterCardWidget.setMaximumSize(QtCore.QSize(540, 120))
        self.FilterCardWidget.setFont(QFont(self.font_family))
        self.FilterCardWidget.setStyleSheet(f"""
            * {{
                font-family: '{self.font_family}';
            }}
        """)

        # 连接信号
        self.FilterCardWidget.btn_filter.clicked.connect(self._handle_filter_clicked)
        self.FilterCardWidget.btn_clear.clicked.connect(self._handle_clear_filter)
        self.FilterCardWidget.switchToAdd.connect(self.show_add_card)
        self.verticalLayout.addWidget(self.FilterCardWidget, 0, QtCore.Qt.AlignCenter)
        self.current_card_widget = self.FilterCardWidget

        self.SHOW_CARD = "FILTER"
        self._update_all_card_icons()  # 新增状态更新
        self._reorder_layout()

    def _remove_current_card(self):
        """移除当前卡片组件"""
        self.verticalLayout.removeWidget(self.current_card_widget)
        self.current_card_widget.deleteLater()
        self.current_card_widget = None

    def _update_scroll_area(self):
        """更新滚动区域高度"""
        count = len(self.card_manager.card_order)
        card_height = 120  # 每个卡片固定高度
        total_height = count * card_height + (count - 1) * 10 + 20  # 包含边距
        self.scrollContent.setMinimumHeight(total_height)
        self.scrollContent.setMaximumHeight(total_height)

    def add_card(self, data, position=None, card_id=None):
        """添加新卡片"""
        card_id = card_id or str(uuid4())

        # 深拷贝数据避免引用问题
        card_data = {
            'id': card_id, 'agent': data.get('agent', ''), 'skin': data.get('skin', ''),
            'model': data.get('model', ''), 'image': data.get('image', ''), 'index': 0
        }

        # 创建自定义卡片
        card_widget = CustomCard(card_id)
        card_widget.dragStarted.connect(lambda w=card_widget: self.handle_drag_start(w))
        card_widget.buttons[1].installEventFilter(card_widget)  # 确保事件过滤器

        card_widget.set_image(card_data['image'])
        card_widget.update_button_icon(self.SHOW_CARD)
        card_widget.set_text(card_data['agent'], card_data['skin'], card_data['model'])
        card_widget.btnClicked.connect(lambda idx, cid=card_id: self._handle_button(cid, idx))

        # 两面包夹芝士！
        if self.SHOW_CARD == "FILTER":
            self._handle_filter_clicked()

        # 确定插入位置
        position = position if position is not None else len(self.card_manager.card_order)

        # 插入到管理器
        self.card_manager.cards[card_id] = {'widget': card_widget, 'data': card_data}
        self.card_manager.card_order.insert(position, card_id)

        # 插入到布局
        self.mainLayout.insertWidget(position, card_widget)

        # 更新索引
        self.card_manager._update_indices(position)
        self._update_scroll_area()
        self.card_manager.card_added.emit(card_id)

        # 两面包夹芝士！
        if self.SHOW_CARD == "FILTER":
            self._handle_filter_clicked()

        return card_id

    def _handle_button(self, card_id, btn_index):
        """处理卡片按钮点击"""
        print(f"卡片 {card_id[:8]} 的按钮 {btn_index} 被点击")
        if btn_index == 0:    # 复制按钮
            self.duplicate_card(card_id)
        elif btn_index == 2:  # 提醒按钮
            self.ringer_card(card_id)
        elif btn_index == 3:  # 删除按钮
            self.remove_card(card_id)
        else:  # 移动按钮
            pass

    def remove_card(self, card_id):
        """删除指定卡片"""
        if card_id not in self.card_manager.cards:
            return False

        # 两面包夹芝士！
        if self.SHOW_CARD == "FILTER":
            self._handle_filter_clicked()

        # 获取位置
        position = self.card_manager.card_order.index(card_id)

        # 移除部件
        widget = self.card_manager.cards[card_id]['widget']
        widget.deleteLater()
        self.mainLayout.removeWidget(widget)

        # 更新数据
        del self.card_manager.cards[card_id]
        self.card_manager.card_order.pop(position)

        # 更新索引
        self.card_manager._update_indices(position)
        self._update_scroll_area()
        self.card_manager.card_removed.emit(card_id)

        # 两面包夹芝士！
        if self.SHOW_CARD == "FILTER":
            self._handle_filter_clicked()
        return True

    def update_card(self, card_id, **kwargs):
        """更新卡片数据"""
        if card_id not in self.card_manager.cards:
            return False

        data = self.card_manager.cards[card_id]['data']
        for key in ['agent', 'skin', 'model', 'image']:
            if key in kwargs:
                data[key] = kwargs[key]

        # 更新UI
        card = self.card_manager.cards[card_id]['widget']
        card.set_text(data['agent'], data['skin'], data['model'])
        if 'image' in kwargs:
            card.set_image(kwargs['image'])
        self.card_manager.card_modified.emit(card_id)
        return True

    def load_brands_data(self):
        """加载JSON数据并初始化干员列表"""
        self.brand_skin_dict = self.json_merger.brand_skin_dict

    def filter_cards(self, agent=None, brand=None, model=None):
        """筛选卡片，支持按品牌筛选"""
        results = []
        for card_id in self.card_manager.card_order:
            data = self.card_manager.cards[card_id]['data']
            match = True

            # 按品牌筛选
            if brand:
                # 检查当前卡片的皮肤是否属于指定品牌
                if data['skin'] not in self.brand_skin_dict.get(brand, []):
                    match = False

            # 按干员筛选
            if agent and data['agent'] != agent:
                match = False

            # 按模型筛选
            if model and data['model'] != model:
                match = False

            if match:
                results.append(card_id)

        return results

    def _handle_filter_clicked(self):
        """处理筛选按钮点击"""
        agent = self.FilterCardWidget.combo_agent.currentText() or None
        brand = self.FilterCardWidget.combo_skin.currentText() or None
        model = self.FilterCardWidget.combo_model.currentText() or None

        # 执行筛选
        filtered_ids = self.filter_cards(agent=agent, brand=brand, model=model)

        # 显示/隐藏卡片
        for card_id in self.card_manager.card_order:
            widget = self.card_manager.cards[card_id]['widget']
            widget.setVisible(card_id in filtered_ids)

        # 啊哈 ~
        self._reorder_layout()
        self._reorder_layout()

    def _handle_clear_filter(self):
        """处理清空筛选操作（删除当前筛选结果）"""
        agent = self.FilterCardWidget.combo_agent.currentText() or None
        brand = self.FilterCardWidget.combo_skin.currentText() or None
        model = self.FilterCardWidget.combo_model.currentText() or None

        # 获取最新筛选结果
        filtered_ids = self.filter_cards(agent=agent, brand=brand, model=model)

        # 批量删除
        for card_id in filtered_ids:
            self.remove_card(card_id)

        # 清空筛选条件
        self.FilterCardWidget.clear_selections()

        # 显示所有剩余卡片
        for card_id in self.card_manager.card_order:
            self.card_manager.cards[card_id]['widget'].show()

        self._reorder_layout()

    def duplicate_card(self, card_id):
        """改进的复制方法"""
        if card_id not in self.card_manager.cards:
            return None

        # 获取原卡片所有数据（包括可能动态添加的属性）
        original_data = copy.deepcopy(self.card_manager.cards[card_id]['data'])

        # 生成新ID并移除原索引
        new_id = str(uuid4())
        original_data.pop('id', None)
        original_data.pop('index', None)

        # 确定插入位置（原位置之后）
        original_pos = self.card_manager.card_order.index(card_id)
        new_position = original_pos + 1

        # 创建新卡片（使用独立的data副本）
        return self.add_card(data=original_data, position=new_position, card_id=new_id)

    def ringer_card(self, card_id):
        print("提醒一下喵 ~")
        return card_id

    def get_card_info(self, card_id):
        """获取卡片完整信息"""
        if card_id not in self.card_manager.cards:
            return None
        return copy.deepcopy(self.card_manager.cards[card_id]['data'])

    def enable_drag_drop(self):
        # 连接卡片拖拽信号
        for card_id in self.card_manager.cards:
            widget = self.card_manager.cards[card_id]['widget']
            widget.dragStarted.connect(lambda w=widget: self.handle_drag_start(w))

        # 设置滚动区域接受拖放
        self.scrollContent.setAcceptDrops(True)
        self.scrollContent.dragEnterEvent = self.dragEnterEvent
        self.scrollContent.dragMoveEvent = self.dragMoveEvent
        self.scrollContent.dropEvent = self.dropEvent

    def _update_all_card_icons(self):
        """更新所有卡片的按钮图标"""
        for card_id in self.card_manager.card_order:
            card = self.card_manager.cards[card_id]['widget']
            card.update_button_icon(self.SHOW_CARD)

    def handle_drag_start(self, card_widget):
        if self.SHOW_CARD == "FILTER":
            return
        self.dragging_card = card_widget  # 记录当前拖拽的卡片
        self.dragging_card.setOpacity(2/3)  # 设置半透明效果

    def dragEnterEvent(self, event):
        if event.mimeData().hasFormat("application/x-card"):
            event.acceptProposedAction()

    def dragMoveEvent(self, event):
        # 计算当前拖拽位置对应的插入点
        pos = event.pos()
        target_index = self._get_drop_index(pos)

        # 更新指示线位置
        if target_index >= 0:
            y_pos = self._get_drop_position(target_index)
            self.drop_line.setGeometry(QtCore.QRect(0, y_pos - 2, self.scrollContent.width(), 3))
            self.drop_line.show()

            # 自动滚动
            scroll_bar = self.SmoothScrollArea.verticalScrollBar()
            viewport_height = self.SmoothScrollArea.viewport().height()  # 获取视口高度
            y_pos = event.pos().y()

            # 获取当前视口在内容中的实际位置
            current_scroll = scroll_bar.value()
            total_height = self.scrollContent.height()

            # 计算当前视口在内容中的中间位置
            viewport_top = current_scroll
            viewport_bottom = current_scroll + viewport_height
            viewport_center = (viewport_top + viewport_bottom) / 2

            # 计算鼠标位置相对于视口的比例
            relative_y = (y_pos * (total_height / (total_height - 80)) - viewport_center) / viewport_height

            # 根据鼠标位置相对于视口中点的位置决定滚动方向和速度
            speed = int(relative_y * 25)
            scroll_bar.setValue(current_scroll + speed)
        else:
            self.drop_line.hide()

        event.accept()

    def dropEvent(self, event):
        mime = event.mimeData()
        if mime.hasFormat("application/x-card"):
            self.drop_line.hide()

        if self.dragging_card is None:
            # 手动触发鼠标释放事件（敲敲暴力版）
            mouse_release_event = QtGui.QMouseEvent(
                QtCore.QEvent.MouseButtonRelease, QtGui.QCursor.pos(),
                QtCore.Qt.LeftButton, QtCore.Qt.LeftButton, QtCore.Qt.NoModifier
            )
            for id, item in self.card_manager.cards.items():
                QtCore.QCoreApplication.instance().sendEvent(item["widget"].buttons[1], mouse_release_event)
            return

        # 获取目标位置
        pos = event.pos()
        target_index = self._get_drop_index(pos)
        if target_index == -1:
            target_index = len(self.card_manager.card_order)

        # 获取当前卡片的ID和原位置
        card_id = self._get_card_id_by_widget(self.dragging_card)
        original_index = self.card_manager.card_order.index(card_id)

        # 移动卡片
        if original_index != target_index:
            # 调整目标索引（当向下拖动时）
            if target_index > original_index:
                target_index -= 1

            # 更新数据
            self.card_manager.card_order.pop(original_index)
            self.card_manager.card_order.insert(target_index, card_id)

            # 重新布局（修了一个小时了真不会呐，反正刷新两次就好了说）
            self._reorder_layout()
            self._reorder_layout()

            # 更新索引
            self.card_manager._update_indices(min(original_index, target_index))

        # 手动触发鼠标释放事件
        mouse_release_event = QtGui.QMouseEvent(
            QtCore.QEvent.MouseButtonRelease, QtGui.QCursor.pos(),
            QtCore.Qt.LeftButton, QtCore.Qt.LeftButton, QtCore.Qt.NoModifier
        )
        QtCore.QCoreApplication.instance().sendEvent(self.dragging_card.buttons[1], mouse_release_event)

        self.dragging_card.setOpacity(1.0)
        self.dragging_card = None

    def _get_drop_index(self, pos):
        # 计算当前鼠标位置对应的插入点
        y = pos.y()
        for i, card_id in enumerate(self.card_manager.card_order):
            widget = self.card_manager.cards[card_id]['widget']
            widget_y = widget.y()
            if y < widget_y + widget.height() / 2:
                return i
        return -1  # 插入到最后

    def _get_drop_position(self, index):
        """获取插入位置的Y坐标（在两卡片中间）"""
        if index == 0:
            return 0
        elif index >= len(self.card_manager.card_order):
            last_widget = self.card_manager.cards[self.card_manager.card_order[-1]]['widget']
            return last_widget.y() + last_widget.height() + 5
        else:
            prev_widget = self.card_manager.cards[self.card_manager.card_order[index-1]]['widget']
            return prev_widget.y() + prev_widget.height() + 5

    def _reorder_layout(self):
        # 根据当前顺序重新排列布局
        temp_order = self.card_manager.card_order.copy()
        self.mainLayout.takeAt(0)  # 移除所有布局项

        for card_id in temp_order:
            widget = self.card_manager.cards[card_id]['widget']
            if widget.isVisible():
                self.mainLayout.addWidget(widget)

        # 按顺序添加可见卡片
        visible_count = 0
        for card_id in self.card_manager.card_order:
            widget = self.card_manager.cards[card_id]['widget']
            if widget.isVisible():
                self.mainLayout.addWidget(widget)
                visible_count += 1
            else:
                widget.hide()

        # 调整滚动区域高度（动态计算）
        card_height = 120  # 单个卡片高度
        spacing = self.mainLayout.spacing()  # 获取布局间距
        margin_top, _, margin_bottom, _ = self.mainLayout.getContentsMargins()
        total_height = (visible_count * card_height + max(0, visible_count - 1) * spacing + margin_top + margin_bottom)

        self.scrollContent.setMinimumHeight(total_height)
        self.scrollContent.setMaximumHeight(total_height)

    def _get_card_id_by_widget(self, widget):
        # 根据部件查找卡片ID
        for card_id, data in self.card_manager.cards.items():
            if data['widget'] is widget:
                return card_id
        return None

    def handle_add_card_request(self, agent, skin, model, image_path):
        """处理添加卡片请求"""
        card_data = {"agent": agent, "skin": skin, "model": model, "image": image_path}
        self.add_card(card_data, 0)  # 添加卡片并自动滑动到顶部
        self.SmoothScrollArea.verticalScrollBar().setValue(0)  # 触发滚动到顶部


class CardWindow(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super().__init__(parent=parent)
        # 初始化卡片框架
        self.ui = Ui_CardFrame()
        self.ui.setupUi(self)

        # 添加测试数据
        self._add_sample_cards()

        # 启用拖拽功能
        self.ui.enable_drag_drop()

        # 设置指示线样式
        self.ui.drop_line.setStyleSheet("""
            QFrame {
                background-color: #0096FF;
                margin: 0 20px;
            }
        """)
        self.ui.scrollContent.layout().addWidget(self.ui.drop_line)

    def _add_sample_cards(self):
        # 添加示例卡片（带图片路径）
        self.ui.add_card({
            'agent': '阿米娅(医疗)',
            'skin': '寰宇独奏',
            'model': '基建',
            'image': r".\resource\img\寰宇独奏.png"
        })
        self.ui.add_card({
            'agent': '艾丽妮',
            'skin': '至高判决',
            'model': '基建',
            'image': r".\resource\img\至高判决.png"
        })
        self.ui.add_card({
            'agent': '澄闪',
            'skin': '夏卉 FA394',
            'model': '基建',
            'image': r".\resource\img\夏卉 FA394.png"
        })
        self.ui.add_card({
            'agent': '空弦',
            'skin': '至虔者荣光',
            'model': '基建',
            'image': r".\resource\img\至虔者荣光.png"
        })
        self.ui.add_card({
            'agent': '铃兰',
            'skin': '弃土花开',
            'model': '基建',
            'image': r".\resource\img\弃土花开.png"
        })
        self.ui.add_card({
            'agent': '薇薇安娜',
            'skin': '寄自奥格尼斯科',
            'model': '基建',
            'image': r".\resource\img\寄自奥格尼斯科.png"
        })
        self.ui.add_card({
            'agent': '羽毛笔',
            'skin': '夏卉 FA210',
            'model': '基建',
            'image': r".\resource\img\夏卉 FA210.png"
        })
        self.ui.add_card({
            'agent': '纯烬艾雅法拉',
            'skin': '远行前的野餐',
            'model': '基建',
            'image': r".\resource\img\远行前的野餐.png"
        })


if __name__ == '__main__':
    app = QtWidgets.QApplication(sys.argv)

    window = CardWindow()
    window.show()
    sys.exit(app.exec_())

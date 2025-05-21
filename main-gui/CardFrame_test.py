# -*- coding: utf-8 -*-
from PyQt5 import QtCore, QtGui, QtWidgets

from DeskpetETO.CustomCard import CustomCard, AddCard
from qfluentwidgets import SmoothScrollArea
from uuid import uuid4
import json
import copy
import sys


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
        CardFrame.setObjectName("CardFrame")
        CardFrame.resize(540, 810)
        CardFrame.setMinimumSize(QtCore.QSize(540, 810))
        CardFrame.setMaximumSize(QtCore.QSize(540, 810))

        font = QtGui.QFont()
        font.setFamily("萝莉体")
        CardFrame.setFont(font)

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

        self.scrollContent = QtWidgets.QWidget()
        self.scrollContent.setMinimumSize(QtCore.QSize(538, 0))
        self.mainLayout = QtWidgets.QVBoxLayout(self.scrollContent)
        self.mainLayout.setContentsMargins(10, 10, 10, 10)
        self.mainLayout.setSpacing(10)

        self.SmoothScrollArea.setWidget(self.scrollContent)
        self.verticalLayout.addWidget(self.SmoothScrollArea)
        self.card_manager = CardManager()

        self.AddCardWidget = AddCard(self.widget)
        self.AddCardWidget.setObjectName("AddCardWidget")
        self.AddCardWidget.setMinimumSize(QtCore.QSize(540, 120))
        self.AddCardWidget.setMaximumSize(QtCore.QSize(540, 120))
        self.verticalLayout.addWidget(self.AddCardWidget, 0, QtCore.Qt.AlignmentFlag.AlignCenter)

        # 拖拽相关初始化
        self.dragging_card = None
        self.drop_line = QtWidgets.QFrame()
        self.drop_line.setFrameShape(QtWidgets.QFrame.HLine)
        self.drop_line.setLineWidth(2)
        self.drop_line.hide()

        # 加载一些乱七八糟的数据
        self.load_brands_data()

        # 设置 AddCard 的信号连接
        self.AddCardWidget.cardRequested.connect(self.handle_add_card_request)

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
            'id': card_id,
            'agent': data.get('agent', ''),
            'skin': data.get('skin', ''),
            'model': data.get('model', ''),
            'image': data.get('image', ''),
            'index': 0
        }

        # 创建自定义卡片
        card_widget = CustomCard(card_id)
        card_widget.dragStarted.connect(lambda w=card_widget: self.handle_drag_start(w))
        card_widget.buttons[1].installEventFilter(card_widget)  # 确保事件过滤器

        card_widget.set_image(card_data['image'])
        card_widget.set_text(card_data['agent'], card_data['skin'], card_data['model'])
        card_widget.btnClicked.connect(lambda idx, cid=card_id: self._handle_button(cid, idx))

        # 确定插入位置
        position = position if position is not None else len(self.card_manager.card_order)

        # 插入到管理器
        self.card_manager.cards[card_id] = {
            'widget': card_widget,
            'data': card_data
        }
        self.card_manager.card_order.insert(position, card_id)

        # 插入到布局
        self.mainLayout.insertWidget(position, card_widget)

        # 更新索引
        self.card_manager._update_indices(position)
        self._update_scroll_area()
        self.card_manager.card_added.emit(card_id)
        return card_id

    def _handle_button(self, card_id, btn_index):
        """处理卡片按钮点击"""
        print(f"卡片 {card_id[:8]} 的按钮 {btn_index} 被点击")
        if btn_index == 0:  # 复制按钮
            self.duplicate_card(card_id)
        elif btn_index == 1:  # 移动按钮
            pass
        elif btn_index == 2:  # 提醒按钮
            self.ringer_card(card_id)
        elif btn_index == 3:  # 删除按钮
            self.remove_card(card_id)

    def remove_card(self, card_id):
        """删除指定卡片"""
        if card_id not in self.card_manager.cards:
            return False

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
        try:
            with open(r".\resource\brands.json", "r", encoding="utf-8") as f:
                self.brand_skin_dict = json.load(f)
                self.brand_skin_dict["默认"] = ["默认"]
        except Exception as e:
            print(f"加载JSON数据失败: {e}")

    def filter_cards(self, agent=None, brand=None, model=None):
        """筛选卡片，支持按品牌筛选"""
        results = []
        for card_id in self.card_manager.card_order:
            data = self.card_manager.cards[card_id]['data']
            match = True

            # 按品牌筛选
            if brand is not None:
                # 检查当前卡片的皮肤是否属于指定品牌
                if data['skin'] not in self.brand_skin_dict.get(brand, []):
                    match = False

            # 按干员筛选
            if agent is not None and data['agent'] != agent:
                match = False

            # 按模型筛选
            if model is not None and data['model'] != model:
                match = False

            if match:
                results.append(card_id)

        return results

    def batch_remove(self, card_ids):
        """批量删除卡片"""
        for card_id in card_ids:
            self.remove_card(card_id)

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
        return self.add_card(
            data=original_data,
            position=new_position,
            card_id=new_id  # 显式指定新ID
        )

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

    def handle_drag_start(self, card_widget):
        # 记录当前拖拽的卡片
        self.dragging_card = card_widget
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
            self.drop_line.setGeometry(
                QtCore.QRect(0, y_pos, self.scrollContent.width(), 3))
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
            viewport_center = viewport_top + viewport_bottom / 2

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
            QtCore.QEvent.MouseButtonRelease,
            QtGui.QCursor.pos(),
            QtCore.Qt.LeftButton,
            QtCore.Qt.LeftButton,
            QtCore.Qt.NoModifier
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
            self.mainLayout.addWidget(widget)

    def _get_card_id_by_widget(self, widget):
        # 根据部件查找卡片ID
        for card_id, data in self.card_manager.cards.items():
            if data['widget'] is widget:
                return card_id
        return None

    def handle_add_card_request(self, agent, skin, model, image_path):
        """处理添加卡片请求"""
        card_data = {
            "agent": agent,
            "skin": skin,
            "model": model,
            "image": image_path
        }
        self.add_card(card_data, 0)  # 添加卡片并自动滑动到顶部
        self.SmoothScrollArea.verticalScrollBar().setValue(0)  # 触发滚动到顶部


class TestWindow(QtWidgets.QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("卡片管理测试")
        self.resize(540, 810)

        # 初始化卡片框架
        self.card_frame = QtWidgets.QWidget(self)
        self.ui = Ui_CardFrame()
        self.ui.setupUi(self.card_frame)
        self.card_frame.setGeometry(0, 0, 540, 810)

        # 添加测试数据
        self._add_sample_cards()

        # 添加控制面板
        # self._setup_controls()

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

    def _setup_controls(self):
        panel = QtWidgets.QWidget(self)
        panel.setGeometry(90, 720, 540, 90)

        # 操作按钮
        btn_delete_all = QtWidgets.QPushButton("删除所有", panel)
        btn_delete_all.clicked.connect(lambda: self.ui.batch_remove(self.ui.card_manager.card_order.copy()))
        btn_delete_all.move(10, 5)


if __name__ == '__main__':
    app = QtWidgets.QApplication(sys.argv)

    window = TestWindow()
    window.show()
    sys.exit(app.exec_())

#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>

// Turn计数全局持久化
static int g_turnMissCount = 0;
void setTurnMissCount(int count) { g_turnMissCount = count; }
int getTurnMissCount() { return g_turnMissCount; }

// 生成自动动画队列
std::vector<std::string> generateRandomAnimQueue(
    float relaxToMoveRatio,
    float specialRatio,
    int totalCount,
    const std::map<std::string, float>& animDurations,
    const std::string& specialName = "Special"
) {
    std::vector<std::string> anims;
    bool hasMove = animDurations.count("Move") > 0;
    bool hasRelax = animDurations.count("Relax") > 0;
    bool hasSpecial = !specialName.empty() && animDurations.count(specialName) > 0;
    if (!hasMove || !hasRelax) return anims;

    float moveDur = animDurations.at("Move");
    float relaxDur = animDurations.at("Relax");
    float specialDur = hasSpecial ? animDurations.at(specialName) : 0.0f;

    // 计算时长比例
    float moveTargetRatio = (1.0f - specialRatio) / (1.0f + relaxToMoveRatio);
    float relaxTargetRatio = relaxToMoveRatio * moveTargetRatio;
    float specialTargetRatio = specialRatio;

    float sumMove = 0, sumRelax = 0, sumSpecial = 0;
    float specialCooldown = 0.0f;
    std::random_device rd;
    std::mt19937 gen(rd());

    // Turn机制变量
    int turnMissCount = g_turnMissCount;
    float turnBaseProb = 0.02f; // 2%
    float turnProbStep = 0.02f; // 每次递增2%
    float turnProb = turnBaseProb;

    for (int i = 0; i < totalCount; ++i) {
        float totalTime = sumMove + sumRelax + sumSpecial;

        float moveGap = moveTargetRatio * totalTime - sumMove;
        float relaxGap = relaxTargetRatio * totalTime - sumRelax;
        float specialGap = hasSpecial ? (specialTargetRatio * totalTime - sumSpecial) : 0.0f;

        // 引入Special冷却机制
        bool canSpecial = hasSpecial && (specialCooldown <= 0.0f);

        float moveWeightNow = moveGap > 0 ? moveGap : 0.0f;
        float relaxWeightNow = relaxGap > 0 ? relaxGap : 0.0f;
        float specialWeightNow = (canSpecial && specialGap > 0) ? specialGap : 0.0f;

        // 增加Move连续性
        float moveBias = 0.67f;
        if (!anims.empty() && anims.back() == "Move") {
            moveWeightNow += moveBias * (moveWeightNow + relaxWeightNow + specialWeightNow);
        }

        float sumWeight = moveWeightNow + relaxWeightNow + specialWeightNow;
        if (sumWeight < 1e-3f) sumWeight = 1.0f;

        float r = std::uniform_real_distribution(0.0f, sumWeight)(gen);
        float acc = 0.0f;
        std::string chosen;

        if ((acc += moveWeightNow) >= r && moveWeightNow > 0) {
            chosen = "Move";
            sumMove += moveDur;
        } else if ((acc + relaxWeightNow) >= r && relaxWeightNow > 0) {
            chosen = "Relax";
            sumRelax += relaxDur;
        } else if (specialWeightNow > 0) {
            chosen = specialName;
            sumSpecial += specialDur;
            specialCooldown = specialDur / 2;
        } else {
            // fallback，按最大gap选
            if (moveGap >= relaxGap && moveGap >= specialGap)
                { chosen = "Move"; sumMove += moveDur; }
            else if (relaxGap >= specialGap)
                { chosen = "Relax"; sumRelax += relaxDur; }
            else
                { chosen = specialName; sumSpecial += specialDur; specialCooldown = specialDur; }
        }
        anims.push_back(chosen);

        // Turn机制
        float turnDraw = std::uniform_real_distribution<float>(0.0f, 1.0f)(gen);
        if (turnDraw < turnProb) {
            anims.push_back("Turn");
            turnMissCount = 0;
            turnProb = turnBaseProb;
        } else {
            turnMissCount++;
            if (turnMissCount > 50) {
                turnProb = turnBaseProb + (turnMissCount - 50) * turnProbStep;
                if (turnProb > 1.0f) turnProb = 1.0f;
            }
        }

        // Special冷却递减
        if (specialCooldown > 0.0f) {
            if (chosen == "Move") specialCooldown -= moveDur;
            else if (chosen == "Relax") specialCooldown -= relaxDur;
            else if (chosen == specialName) specialCooldown -= specialDur;
        }
    }
    g_turnMissCount = turnMissCount;
    return anims;
}

// 带前缀生成（继承Turn计数，prefix最后一次Turn后计数）
std::vector<std::string> generateRandomAnimQueueWithPrefix(
    const std::vector<std::string>& prefix,
    float relaxToMoveRatio,
    float specialRatio,
    int count,
    const std::map<std::string, float>& animDurations,
    const std::string& specialName
) {
    // 统计prefix末尾到最后一个Turn的距离
    int turnMiss = 0;
    for (int i = static_cast<int>(prefix.size()) - 1; i >= 0; --i) {
        if (prefix[i] == "Turn") break;
        ++turnMiss;
    }
    setTurnMissCount(turnMiss);
    return generateRandomAnimQueue(relaxToMoveRatio, specialRatio, count, animDurations, specialName);
}

// 测试打印前100个动作及累计时长
void printRandomAnimQueueTest(
    float relaxToMoveRatio,
    float specialRatio,
    int totalCount,
    const std::map<std::string, float>& animDurations,
    const std::string& specialName = "Special"
) {
    auto queue = generateRandomAnimQueue(relaxToMoveRatio, specialRatio, totalCount, animDurations, specialName);
    std::map<std::string, float> totalTime;
    std::cout << "动作序列: ";
    for (size_t i = 0; i < queue.size(); ++i) {
        std::cout << queue[i];
        if (i != queue.size() - 1) std::cout << ", ";
        auto it = animDurations.find(queue[i]);
        if (it != animDurations.end())
            totalTime[queue[i]] += it->second;
    }
    std::cout << "\n\n各动作累计时长:\n";
    for (const auto& [name, t] : totalTime) {
        std::cout << std::setw(8) << name << ": " << t << "s\n";
    }
    std::cout << std::setw(8) << "Turn" << ": " << queue.size() - totalCount << " it\n";
}

struct ActiveParams {
    float specialRatio;      // Special:(Move+Relax+Special)的目标时长比例
    float relaxToMoveRatio;  // Relax:Move的目标时长比例
};

ActiveParams getActiveParams(int activeLevel) {
    // 限制范围
    if (activeLevel < 0) activeLevel = 0;
    if (activeLevel > 6) activeLevel = 6;

    // 采用对数平滑插值
    float specialTable[7] = { 0.0f, 1.0f / 9.0f, 1.0f / 7.7f, 1.0f / 6.5f, 1.0f / 5.5f, 1.0f / 4.7f, 1.0f / 4.0f };

    // Relax:Move比例: 3(0), 2(1), 1.5(2), 1(3), 0.67(4), 0.5(5), 0.33(6)
    float relaxMoveTable[7] = { 3.0f, 2.0f, 1.5f, 1.0f, 0.67f, 0.5f, 0.33f };

    return {specialTable[activeLevel], relaxMoveTable[activeLevel]};
}

int test_main() {
    system("chcp 65001");

    // 动画时长数据
    std::map<std::string, float> animDurations = {
        {"Default", 0.0f},
        {"Interact", 1.33333f},
        {"Move", 1.0f},
        {"Relax", 2.66667f},
        {"Sit", 4.0f},
        {"Sleep", 3.0f},
        {"Special", 12.0f}
    };

    int activeLevel = 2; // 活跃系数，0-6
    ActiveParams params = getActiveParams(activeLevel);

    printRandomAnimQueueTest(params.relaxToMoveRatio, params.specialRatio, 128, animDurations);

    return 0;
}

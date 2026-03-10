#pragma once
#include "imu_shared.h"

// Запускает визуализатор (блокирующий вызов, до закрытия окна).
// Читает из buf.queue, интегрирует IMU, рендерит в реальном времени.
void run_visualizer(SharedBuffer& buf);
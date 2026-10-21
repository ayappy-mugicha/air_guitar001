import unittest

from supervise import (
    MAX_CONSECUTIVE_FAILURES,
    STABLE_UPTIME_S,
    on_child_exit,
    stable_uptime_reached,
)


class SuperviseLogicTest(unittest.TestCase):
    def test_stable_uptime_reached(self):
        self.assertFalse(stable_uptime_reached(STABLE_UPTIME_S - 0.1))
        self.assertTrue(stable_uptime_reached(STABLE_UPTIME_S))

    def test_early_death_increments_failures(self):
        restart, failures = on_child_exit(0, 5.0)
        self.assertTrue(restart)
        self.assertEqual(failures, 1)

    def test_three_early_deaths_stop_restart(self):
        restart, failures = on_child_exit(
            MAX_CONSECUTIVE_FAILURES - 1, STABLE_UPTIME_S - 1
        )
        self.assertFalse(restart)
        self.assertEqual(failures, MAX_CONSECUTIVE_FAILURES)

    def test_late_death_does_not_increment(self):
        restart, failures = on_child_exit(2, STABLE_UPTIME_S + 10)
        self.assertTrue(restart)
        self.assertEqual(failures, 2)


if __name__ == "__main__":
    unittest.main()

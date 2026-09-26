import unittest

import numpy as np
import pandas as pd
from anomaly_detector import preprocess_data, train_anomaly_detector


class AnalyticsTests(unittest.TestCase):
    def readings(self):
        return pd.DataFrame({
            'timestamp': pd.date_range('2026-01-01', periods=24, freq='h'),
            'tank_level_pct': np.linspace(90, 40, 24),
            'flow_rate_lpm': [0.0] * 23 + [30.0],
        })

    def test_training_preprocesses_raw_input_without_mutating_it(self):
        raw = self.readings()
        result, model = train_anomaly_detector(raw)
        self.assertNotIn('is_night', raw)
        self.assertEqual(len(result), 24)
        self.assertEqual(result.loc[result.is_night.eq(1)].index.tolist(), [1, 2, 3, 4, 5])
        self.assertTrue(result.iloc[-1].is_anomaly)
        self.assertTrue(np.isfinite(result.anomaly_score).all())

    def test_invalid_values_are_rejected(self):
        for column, value in [('tank_level_pct', -1), ('tank_level_pct', 101),
                              ('tank_level_pct', np.nan), ('flow_rate_lpm', np.inf),
                              ('flow_rate_lpm', -1), ('timestamp', None),
                              ('timestamp', 'not a date')]:
            with self.subTest(column=column, value=value):
                raw = self.readings().astype({'timestamp': object})
                raw.loc[0, column] = value
                with self.assertRaises(ValueError):
                    preprocess_data(raw)

    def test_missing_columns_and_insufficient_rows(self):
        for raw in [self.readings().drop(columns='timestamp'),
                    self.readings().iloc[:0], self.readings().iloc[:1]]:
            with self.assertRaises(ValueError):
                train_anomaly_detector(raw)


if __name__ == '__main__':
    unittest.main()

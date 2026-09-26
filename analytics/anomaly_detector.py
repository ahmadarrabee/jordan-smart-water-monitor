"""Exploratory anomaly detection, not a validated leak alarm."""
import argparse
from pathlib import Path
import numpy as np
import pandas as pd
from sklearn.ensemble import IsolationForest

FEATURES = ['tank_level_pct', 'flow_rate_lpm', 'is_night']


def preprocess_data(df):
    df = df.copy()
    missing = {'timestamp', *FEATURES[:2]} - set(df.columns)
    if missing:
        raise ValueError(f'Missing columns: {sorted(missing)}')
    if df.empty:
        raise ValueError('Telemetry must contain rows')
    df['timestamp'] = pd.to_datetime(df['timestamp'], errors='raise')
    if df['timestamp'].isna().any():
        raise ValueError('Missing timestamps are not allowed')
    for column in FEATURES[:2]:
        df[column] = pd.to_numeric(df[column], errors='raise')
        if not np.isfinite(df[column].to_numpy(dtype=float)).all():
            raise ValueError(f'{column} must contain finite values')
    if not df['tank_level_pct'].between(0, 100).all():
        raise ValueError('Tank level must be between 0 and 100')
    if (df['flow_rate_lpm'] < 0).any():
        raise ValueError('Flow cannot be negative')
    df['is_night'] = df['timestamp'].dt.hour.between(1, 5).astype(int)
    return df


def load_and_preprocess_data(filepath):
    return preprocess_data(pd.read_csv(filepath))


def train_anomaly_detector(df):
    df = preprocess_data(df)
    if len(df) < 2:
        raise ValueError('Training requires at least two readings')
    model = IsolationForest(n_estimators=100, contamination=0.05, random_state=42)
    df['anomaly_label'] = model.fit_predict(df[FEATURES])
    df['anomaly_score'] = model.decision_function(df[FEATURES])
    df['is_anomaly'] = df['anomaly_label'].eq(-1)
    return df, model


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('csv', nargs='?', type=Path,
                        default=Path(__file__).parent / 'data' / 'sample_telemetry.csv')
    args = parser.parse_args()
    result, model = train_anomaly_detector(load_and_preprocess_data(args.csv))
    print('Potential anomalies in the training data (demonstration only):')
    print(result.loc[result['is_anomaly'], ['timestamp', *FEATURES[:2]]])

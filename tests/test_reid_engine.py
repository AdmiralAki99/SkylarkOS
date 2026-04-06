import sys
import os
import numpy as np
import pytest
from unittest.mock import MagicMock, patch

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../ws/src/skylark_identity'))
from skylark_identity.reid_engine import ReIDEngine


@pytest.fixture
def engine(tmp_path):
    # mock the session so we don't need the actual onnx file
    model_path = tmp_path / "osnet_x0_25.onnx"
    model_path.touch()
    embedding_path = tmp_path / "reid_embedding.npy"

    mock_session = MagicMock()
    with patch("onnxruntime.InferenceSession", return_value=mock_session):
        eng = ReIDEngine(
            model_path=str(model_path),
            embedding_path=str(embedding_path)
        )
    eng.session = mock_session
    return eng


def make_frame(h=480, w=640):
    return np.random.randint(0, 255, (h, w, 3), dtype=np.uint8)

def make_bbox(x1=0.2, y1=0.1, x2=0.6, y2=0.9):
    return (x1, y1, x2, y2)

def make_unit_embedding(dim=512, seed=42):
    rng = np.random.default_rng(seed)
    v = rng.standard_normal(dim).astype(np.float32)
    return v / np.linalg.norm(v)


class TestEnrollmentState:
    def test_not_enrolled_before_enroll(self, engine):
        assert engine.is_enrolled() is False

    def test_enrolled_after_enroll(self, engine):
        emb = make_unit_embedding()
        engine.session.run.return_value = [emb.reshape(1, -1)]
        engine.enroll([(make_frame(), make_bbox())])
        assert engine.is_enrolled() is True

    def test_empty_crops_does_not_enroll(self, engine):
        engine.enroll([])
        assert engine.is_enrolled() is False


class TestPreprocess:
    def test_output_shape(self, engine):
        tensor = engine._preprocess(make_frame(), make_bbox())
        assert tensor is not None
        assert tensor.shape == (1, 3, 256, 128)  # NCHW

    def test_output_dtype(self, engine):
        tensor = engine._preprocess(make_frame(), make_bbox())
        assert tensor.dtype == np.float32

    def test_empty_crop_returns_none(self, engine):
        result = engine._preprocess(make_frame(), (0.5, 0.5, 0.5, 0.5))
        assert result is None

    def test_out_of_bounds_bbox_is_clamped(self, engine):
        # bbox going outside frame bounds should be clamped not crash
        tensor = engine._preprocess(make_frame(), (-0.5, -0.5, 1.5, 1.5))
        assert tensor is not None
        assert tensor.shape == (1, 3, 256, 128)

    def test_values_normalized(self, engine):
        frame = np.full((480, 640, 3), 128, dtype=np.uint8)
        tensor = engine._preprocess(frame, make_bbox())
        # after imagenet norm values should be in a sane range
        assert tensor.min() > -5.0
        assert tensor.max() < 5.0


class TestMatch:
    def test_match_returns_negative_when_not_enrolled(self, engine):
        score = engine.match(make_frame(), make_bbox())
        assert score == -1.0

    def test_match_returns_negative_on_empty_crop(self, engine):
        emb = make_unit_embedding()
        engine.owner_embeddings = emb
        result = engine.match(make_frame(), (0.5, 0.5, 0.5, 0.5))
        assert result == -1.0

    def test_perfect_match_score_is_one(self, engine):
        emb = make_unit_embedding()
        engine.owner_embeddings = emb
        engine.session.run.return_value = [emb.reshape(1, -1)]
        score = engine.match(make_frame(), make_bbox())
        assert abs(score - 1.0) < 1e-5

    def test_orthogonal_embeddings_score_near_zero(self, engine):
        # two orthogonal unit vectors should have dot product = 0
        emb_a = np.zeros(512, dtype=np.float32)
        emb_b = np.zeros(512, dtype=np.float32)
        emb_a[0] = 1.0
        emb_b[1] = 1.0
        engine.owner_embeddings = emb_a
        engine.session.run.return_value = [emb_b.reshape(1, -1)]
        score = engine.match(make_frame(), make_bbox())
        assert abs(score) < 1e-5

    def test_match_score_is_float(self, engine):
        emb = make_unit_embedding()
        engine.owner_embeddings = emb
        engine.session.run.return_value = [emb.reshape(1, -1)]
        score = engine.match(make_frame(), make_bbox())
        assert isinstance(score, float)


class TestEnrollAveraging:
    def test_enrollment_produces_unit_norm(self, engine):
        emb = make_unit_embedding()
        engine.session.run.return_value = [emb.reshape(1, -1)]
        pairs = [(make_frame(), make_bbox()) for _ in range(5)]
        engine.enroll(pairs)
        assert abs(np.linalg.norm(engine.owner_embeddings) - 1.0) < 1e-5

    def test_enrollment_persists_to_disk(self, engine, tmp_path):
        emb = make_unit_embedding()
        engine.session.run.return_value = [emb.reshape(1, -1)]
        engine.enroll([(make_frame(), make_bbox())])
        assert engine.embedding_path.exists()

    def test_load_embedding_restores_state(self, engine):
        emb = make_unit_embedding()
        np.save(engine.embedding_path, emb)
        engine.owner_embeddings = None
        engine._load_embedding(engine.embedding_path)
        assert engine.owner_embeddings is not None
        np.testing.assert_array_almost_equal(engine.owner_embeddings, emb)

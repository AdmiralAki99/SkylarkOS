# tools/export_osnet.py
import torchreid
import torch

model = torchreid.models.build_model(
    name='osnet_x0_25',
    num_classes=1000,
    pretrained=True
)
model.eval()

dummy_input = torch.randn(1, 3, 256, 128)

torch.onnx.export(
    model,
    dummy_input,
    'models/osnet_x0_25.onnx',
    input_names=['input'],
    output_names=['output'],
    dynamic_axes={'input': {0: 'batch_size'}},
    opset_version=11
)
print("Exported to data/osnet_x0_25.onnx")

import pytest
from resdata import ResDataType
from resdata.grid import GridGenerator
from resdata.grid.faults import FaultBlock, FaultBlockLayer
from resdata.resfile import ResdataKW


def make_grid(nx=4, ny=3, nz=2):
    return GridGenerator.create_rectangular((nx, ny, nz), (1, 1, 1))


def make_kw(grid, values=None, default=0):
    kw = ResdataKW("FAULTBLK", grid.get_global_size(), ResDataType.RD_INT)
    kw.assign(default)
    if values:
        nx = grid.get_nx()
        ny = grid.get_ny()
        for (i, j, k), value in values.items():
            kw[i + j * nx + k * nx * ny] = value
    return kw


def block_ids(layer):
    return [block.getBlockID() for block in layer]


def test_empty_layer_len_and_k_accessors():
    grid = make_grid(nz=3)
    layer = FaultBlockLayer(grid, 2)

    assert len(layer) == 0
    assert layer.getK() == 2
    assert layer.get_k() == 2
    assert layer.k == 2


def test_empty_layer_index_errors_and_type_error():
    layer = FaultBlockLayer(make_grid(), 0)

    with pytest.raises(IndexError):
        layer[0]
    with pytest.raises(IndexError):
        layer[-1]
    with pytest.raises(TypeError):
        layer["0"]


def test_add_block_default_id_retrievable_by_index_and_id():
    layer = FaultBlockLayer(make_grid(), 0)

    block = layer.addBlock()

    assert isinstance(block, FaultBlock)
    assert block.getBlockID() == 1
    assert len(layer) == 1
    assert 1 in layer
    assert layer[0].getBlockID() == 1
    assert layer.getBlock(1).getBlockID() == 1


def test_add_block_explicit_sparse_id_controls_next_id():
    layer = FaultBlockLayer(make_grid(), 0)

    block = layer.addBlock(7)

    assert block.getBlockID() == 7
    assert len(layer) == 1
    assert 7 in layer
    assert 6 not in layer
    assert layer.getNextID() == 8


def test_get_block_uses_block_id_while_getitem_uses_storage_index():
    layer = FaultBlockLayer(make_grid(), 0)
    layer.addBlock(4)
    layer.addBlock(9)

    assert layer[0].getBlockID() == 4
    assert layer[1].getBlockID() == 9
    assert layer.getBlock(4).getBlockID() == 4
    assert layer.getBlock(9).getBlockID() == 9
    with pytest.raises(IndexError):
        layer[4]


def test_delete_block_updates_len_contains_and_lookup():
    layer = FaultBlockLayer(make_grid(), 0)
    layer.addBlock(2)
    layer.addBlock(5)

    layer.deleteBlock(2)

    assert len(layer) == 1
    assert 2 not in layer
    assert 5 in layer
    assert layer[0].getBlockID() == 5
    with pytest.raises(KeyError):
        layer.getBlock(2)


def test_duplicate_add_block_raises_key_error():
    layer = FaultBlockLayer(make_grid(), 0)
    layer.addBlock(3)

    with pytest.raises(KeyError):
        layer.addBlock(3)

    assert len(layer) == 1
    assert layer.getBlock(3).getBlockID() == 3


def test_iteration_yields_fault_blocks_in_storage_order():
    layer = FaultBlockLayer(make_grid(), 0)
    layer.addBlock(3)
    layer.addBlock(8)

    blocks = list(layer)

    assert [type(block) for block in blocks] == [FaultBlock, FaultBlock]
    assert [block.getBlockID() for block in blocks] == [3, 8]


def test_load_keyword_preserves_positive_block_ids_and_ignores_zero():
    grid = make_grid(nx=3, ny=3, nz=1)
    kw = make_kw(grid, {(0, 0, 0): 4, (2, 1, 0): 9, (1, 2, 0): 4})
    layer = FaultBlockLayer(grid, 0)

    layer.loadKeyword(kw)

    assert len(layer) == 2
    assert set(block_ids(layer)) == {4, 9}
    assert 0 not in layer
    assert layer[0, 0].getBlockID() == 4
    assert layer[2, 1].getBlockID() == 9
    with pytest.raises(ValueError):
        layer[1, 1]


def test_scan_keyword_reorders_to_connected_blocks_and_validates_size():
    grid = make_grid(nx=3, ny=2, nz=1)
    kw = make_kw(grid, default=5)
    layer = FaultBlockLayer(grid, 0)

    layer.scanKeyword(kw)

    assert len(layer) == 1
    assert layer.getNextID() == 2
    assert layer[0].getBlockID() == 1
    assert layer[2, 1].getBlockID() == 1

    wrong_size_kw = ResdataKW(
        "FAULTBLK", grid.get_global_size() - 1, ResDataType.RD_INT
    )
    with pytest.raises(ValueError):
        FaultBlockLayer(grid, 0).scanKeyword(wrong_size_kw)

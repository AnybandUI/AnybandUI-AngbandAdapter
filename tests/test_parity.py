"""Compare complete gameplay views from identical saves on old and extracted engines."""
import argparse
import json
from pathlib import Path
import shutil
import tempfile
from types import SimpleNamespace
import test_backend as protocol

def configure(backend):
    protocol.ARGS = SimpleNamespace(backend=backend, data=backend.parent/'lib', verbose=False)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--backend', type=Path, required=True)
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    backend, reference = args.backend.resolve(), args.reference.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='parity-', dir=args.output) as folder:
        folder = Path(folder)
        configure(reference)
        seed = protocol.Engine(folder/'seed')
        try:
            seed.hello(native_inventory=True, native_equipment=True); seed.birth()
            seed.fixture_command('A')
            for _ in range(3): seed.fixture_command('n', ['blubbering idiot'])
            seed.call('session.close'); assert seed.process.wait(timeout=10) == 0
        finally:
            seed.stop()
        def play(executable, name):
            profile = folder/name
            shutil.copytree(folder/'seed', profile)
            configure(executable)
            e = protocol.Engine(profile)
            views = []
            try:
                e.hello(native_inventory=True, native_equipment=True)
                e.call('session.load', {'save':'ProtocolTest'}); e.next_state(None)
                def settle():
                    for _ in range(30):
                        if e.state['readiness'] == 'ready': return
                        assert not e.prompt, e.prompt
                        e.key('enter')
                    raise AssertionError('Session did not settle')
                def snapshot():
                    state = e.call('state.get')['result']
                    selected = {key:state[key] for key in ('turn','player','items','monsters','map','dungeon','messages')}
                    selected = json.loads(json.dumps(selected))
                    for key in ('items','monsters'):
                        for item in selected[key]: item.pop('id', None)
                    views.append(selected)
                    return state
                settle(); snapshot()
                e.key(ord('i')); e.key(ord('e'))
                assert (e.inventory_events,e.equipment_events) == (1,1)
                e.key(ord('R')); prompt=e.wait_prompt(); old=e.state['revision']; e.prompt=None
                e.call('prompt.reply', {'prompt_id':prompt['prompt_id'],'value':None}); e.next_state(old)
                settle(); snapshot()
                for mode in range(1,7):
                    assert 'result' in e.call('dungeon.tiles', {'id':mode})
                    snapshot()
                e.call('dungeon.tiles', {'id':0})
                e.call('dungeon.camera', {'enabled':True}); snapshot()
                e.call('dungeon.camera', {'enabled':False})
                for _ in range(40):
                    e.key(ord(',')); settle(); snapshot()
                e.key(ord('>')); settle()
                state=snapshot()
                fixture = {'state':state, 'catalog':e.call('catalog.get')['result'],
                    'commands':e.call('commands.list')['result']}
                (args.output/(name+'-fixture.json')).write_text(json.dumps(fixture), encoding='utf-8')
                e.call('session.close'); assert e.process.wait(timeout=10) == 0
            finally:
                e.stop()
            return {"views":views, "motion":e.motion_events}
        original = play(reference, 'reference')
        current = play(backend, 'adapter')
        if original != current:
            for name,value in (('reference',original),('adapter',current)):
                (args.output/(name+'-states.json')).write_text(json.dumps(value,indent=2))
            raise AssertionError('Gameplay views differ; see saved state sequences')
        assert any(not fx['blink'] for batch in current['motion'] for fx in batch['effects']), 'No walking observations exercised'
        print(f'PASS: {len(current["views"])} identical gameplay views and matching motion events, native browsers, rest cancellation, '
              'six tilesets, camera, waiting and level transition')

if __name__ == '__main__':
    main()

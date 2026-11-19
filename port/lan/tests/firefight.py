"""Run two real Windows games through a lossy UDP relay. Requires prepared NTSC-U assets."""
import argparse
import os
from pathlib import Path
import re
import shutil
import socket
import subprocess
import tempfile
import threading
import time


def relay(sock, host, stop, interrupt):
    client = None
    count = 0
    previous = None
    resume_at = 0
    sock.settimeout(.1)
    while not stop.is_set():
        try:
            packet, source = sock.recvfrom(4096)
        except socket.timeout:
            continue
        if interrupt.is_set():
            interrupt.clear()
            resume_at = time.monotonic() + 10
            previous = None
        if time.monotonic() < resume_at:
            continue
        if source == host:
            destination = client
        else:
            client, destination = source, host
        count += 1
        if destination is None or count % 9 == 0:
            continue
        sock.sendto(packet, destination)
        if previous:
            sock.sendto(*previous)  # delayed duplicate, after a newer packet
            previous = None
        if count % 3 == 0:
            previous = packet, destination


def game_data(source, destination):
    for name in ('ge007.ntsc-final.z64', 'pcmodels-ntsc-final', 'pccg-ntsc-final'):
        asset = source / name
        if not asset.exists():
            raise FileNotFoundError(asset)
        for f in asset.rglob('*') if asset.is_dir() else [asset]:
            if f.is_file():
                target = destination / f.relative_to(source)
                target.parent.mkdir(parents=True, exist_ok=True)
                if not target.exists():
                    try:
                        os.link(f, target)
                    except OSError:
                        shutil.copyfile(f, target)


def pickup_passed(texts, kind):
    host, client = texts
    awards = re.findall(r'LAN contested pickup (\d+) awarded to ([01])', host)
    if len(awards) != 1 or 'LAN contested pickup' in client:
        return False
    command, winner = awards[0]
    for text in texts:
        if re.findall(rf'LAN local pickup {command} hidden ([01])', text) != ['0', '1', '0']:
            return False
    seats = []
    for log_number, text in enumerate(texts):
        local = {}
        for seat, low, high, ammo in re.findall(r'LAN local inventory (\d) owned (\w+) (\w+) ammo (\d+)', text):
            local[int(seat) ^ log_number] = (int(high + low, 16), int(ammo))
        if len(local) != 2:
            return False
        seats.append(local)
    if seats[0] != seats[1]:
        return False
    owned, ammo = seats[0][int(winner)]
    other, baseline = seats[0][1-int(winner)]
    new_weapons = owned ^ other
    if kind == 'weapon' and (new_weapons.bit_count() != 1 or other & new_weapons):
        return False
    if kind == 'ammo' and new_weapons:
        return False
    return baseline == 52 and ammo == baseline + (10 if kind == 'weapon' else 50)


def drop_passed(texts, shooter):
    host, client = texts
    seat = int(shooter == 'client')
    awards = re.findall(r'LAN awarded drop (\d+) item 4 seat (\d) pistolAmmo (\d+)', host)
    if len(awards) != 1 or 'LAN awarded drop' in client:
        return False
    generation, winner, ammo = awards[0]
    if int(winner) != seat:
        return False
    for log, action, local_seat in ((host, 'created', seat), (client, 'mirrored', 1-seat)):
        token = f'drop {generation} item 4'
        if log.count('LAN ' + action + ' ' + token + '\n') != 1:
            return False
        removal = 'LAN removed ' + token + '\n'
        if log.count(removal) != 1:
            return False
        pattern = rf'LAN seat {local_seat} .*weapon 4 pistolAmmo (\d+)'
        totals = re.findall(pattern, log.split('LAN ' + action + ' ' + token)[-1])
        if not totals or totals[-1] != ammo:
            return False
        # The remote native seat can tick immediately before local seat zero
        # applies the same packet's removal. Either order is valid.
        if action == 'created':
            before = re.findall(pattern, log.split('LAN awarded ' + token)[0])
            if not before or int(ammo) - int(before[-1]) != 10:
                return False
    return True


def verify(binary, data, root, logs_dir, shooter, reconnect=False, pickup=None, rematch=False, dropped=False):
    case = pickup or (f'{shooter}-drop' if dropped else f'{shooter}-rematch' if rematch else
                      f'{shooter}-reconnect' if reconnect else shooter)
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as reserve:
        reserve.bind(('127.0.0.1', 0))
        host = reserve.getsockname()
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as proxy:
        proxy.bind(('127.0.0.1', 0))
        stop = threading.Event()
        interrupt = threading.Event()
        interrupted = False
        worker = threading.Thread(target=relay, args=(proxy, host, stop, interrupt))
        worker.start()
        games = []
        logs = []
        try:
            for role in ('host', 'client'):
                directory = root / case / role
                game_data(data, directory / 'data')
                script = ';'.join(f'{4500+i*450}:Z' for i in range(22)) if role == shooter else (
                    ';'.join(f'{14000+i*550}:A' for i in range(32)))
                env = dict(os.environ, GE_INPUTSCRIPT=script, GE_INPUTSCRIPT_MS='1', GE_LAN_TRACE='1')
                if dropped:
                    if role == shooter:
                        env['GE_LAN_TEST_DROP'] = '1'
                    else:
                        env['GE_INPUTSCRIPT'] = '60000:A'
                if pickup:
                    env.update(GE_LAN_TEST_PICKUP=pickup, GE_INPUTSCRIPT='60000:A')
                args = ['--lan-host', '--lan-port', str(host[1])] if role == 'host' else [
                    '--lan-join', '127.0.0.1', '--lan-port', str(proxy.getsockname()[1])]
                if rematch:
                    if role == 'host':
                        args += ['--lan-score-limit', '1']
                    start = 16000 if role == 'host' else 21000
                    presses = ';'.join(f'{start+800*i}:START' for i in range(16))
                    env['GE_INPUTSCRIPT'] = (script + ';' if role == shooter else '') + presses
                log = (logs_dir / f'{case}-firing-{role}.log').open('wb')
                logs.append(log)
                games.append(subprocess.Popen([str(binary), *args], cwd=directory, env=env,
                                              stdout=log, stderr=subprocess.STDOUT))
            deadline = time.monotonic() + 45
            while time.monotonic() < deadline:
                passed = []
                texts = []
                for role, game, log in zip(('host', 'client'), games, logs):
                    if game.poll() is not None:
                        raise RuntimeError(f'{role} exited: {log.name}')
                    text = Path(log.name).read_text(errors='replace')
                    if rematch:
                        rounds = re.findall(r'LAN round (\d+) started arena 2 score limit 1', text)
                        won = (1, 0) if shooter == 'host' else (0, 1)
                        scores = re.findall(r'LAN round (\d+) results host (\d+) guest (\d+)', text)
                        if len(rounds) == 2 and scores == [(rounds[0], str(won[0]), str(won[1]))]:
                            second = text.split(f'LAN round {rounds[1]} started')[-1]
                            if all(re.search(rf'LAN seat {s} life 1 hp 1\.000 dead 0 kills 0 .*weapon 4', second)
                                   for s in range(2)):
                                texts.append(tuple(rounds))
                        continue
                    if pickup or dropped:
                        texts.append(text)
                        continue
                    attacker = int(role != shooter)
                    victim = 1 - attacker
                    kills = re.findall(rf'LAN seat {attacker} .*? kills (\d+)', text)
                    dead = re.search(rf'LAN seat {victim} life 1 hp [-.\d]+ dead 1', text)
                    revived = re.search(rf'LAN seat {victim} life 2 hp 1\.000 dead 0', text)
                    if reconnect and role == 'client' and dead and not interrupted:
                        interrupt.set()
                        interrupted = True
                    rejoined = not reconnect or ('peer timeout' in text and text.count('nonce=') >= 2)
                    passed.append(dead and revived and rejoined and max(map(int, kills), default=0) == 1)
                if dropped and drop_passed(texts, shooter):
                    print(f'{case}: one native PP7 drop, matching collection and ammo on both games', flush=True)
                    return
                if pickup and pickup_passed(texts, pickup):
                    print(f'{pickup}: one award, matching inventories and native item regeneration', flush=True)
                    return
                if rematch and len(texts) == 2 and texts[0] == texts[1] and texts[0][0] != texts[0][1]:
                    print(f'{case}: both games showed the final score and began one fresh round', flush=True)
                    return
                if passed and all(passed):
                    print(f'{case}: both processes confirmed one kill, death and respawn', flush=True)
                    return
                time.sleep(.25)
            raise AssertionError(f'Combat state failed to converge: {logs_dir}')
        finally:
            stop.set()
            worker.join()
            for game in games:
                game.terminate()
                game.wait()
            for log in logs:
                log.close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary', type=Path)
    parser.add_argument('data', type=Path)
    parser.add_argument('--pickups-only', action='store_true')
    parser.add_argument('--rounds-only', action='store_true')
    parser.add_argument('--drops-only', action='store_true')
    args = parser.parse_args()
    logs_dir = Path(tempfile.mkdtemp(prefix='ge007-firefight-logs-'))
    print(f'Test logs: {logs_dir}', flush=True)
    with tempfile.TemporaryDirectory(prefix='ge007-firefight-') as work:
        if not args.pickups_only and not args.rounds_only and not args.drops_only:
            for shooter in ('host', 'client'):
                verify(args.binary.resolve(), args.data.resolve(), Path(work), logs_dir, shooter)
            verify(args.binary.resolve(), args.data.resolve(), Path(work), logs_dir, 'host', reconnect=True)
        if not args.rounds_only and not args.drops_only:
            for kind in ('weapon', 'ammo'):
                verify(args.binary.resolve(), args.data.resolve(), Path(work), logs_dir, 'host', pickup=kind)
        if not args.pickups_only and not args.drops_only:
            for shooter in ('host', 'client'):
                verify(args.binary.resolve(), args.data.resolve(), Path(work), logs_dir, shooter, rematch=True)
        if not args.pickups_only and not args.rounds_only:
            for shooter in ('host', 'client'):
                verify(args.binary.resolve(), args.data.resolve(), Path(work), logs_dir, shooter, dropped=True)

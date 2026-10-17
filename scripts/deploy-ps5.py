"""Sends a Manta build to the PS5 over FTP, or fetches its log.

    python scripts/deploy-ps5.py <PS5-IP>              upload hub/dist/PPSA98265
    python scripts/deploy-ps5.py <PS5-IP> --log        fetch /data/emuland/hub.log
    python scripts/deploy-ps5.py <PS5-IP> --app app    the console test app (teste.log)
    python scripts/deploy-ps5.py <PS5-IP> --folder PPSA98265
                                                       a release, as extracted from its ZIP

Needs an FTP server already running on the console (etaHEN's is on port 2121).
Every folder and file gets SITE CHMOD 777, as PSXS5's tool does: a title whose
eboot.bin is not executable fails with "Can't start the game or app"
(CE-107750-0).
"""

import argparse
import ftplib
import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent


def chmod(ftp: ftplib.FTP, path: str) -> None:
    try:
        ftp.sendcmd(f"SITE CHMOD 777 {path}")
    except ftplib.all_errors:
        pass


def mkdirs(ftp: ftplib.FTP, path: str) -> None:
    current = ""
    for part in path.strip("/").split("/"):
        current += "/" + part
        try:
            ftp.mkd(current)
        except ftplib.error_perm:
            pass
    chmod(ftp, path)


def upload(ftp: ftplib.FTP, local: pathlib.Path, remote: str) -> int:
    mkdirs(ftp, remote)
    count = 0
    # eboot.bin and param.json go last, so a half-sent title never looks complete.
    files = sorted(local.rglob("*"), key=lambda p: (p.name in ("eboot.bin", "param.json"), str(p)))
    for item in files:
        target = remote + "/" + item.relative_to(local).as_posix()
        if item.is_dir():
            mkdirs(ftp, target)
            continue
        mkdirs(ftp, target.rsplit("/", 1)[0])
        with item.open("rb") as source:
            ftp.storbinary(f"STOR {target}", source)
        chmod(ftp, target)
        count += 1
        print(f"  {target}")
    return count


# The home screen reads the icon, the backgrounds and the name from the copies
# the system made when the app was registered (/user/appmeta/<id>, and
# /user/app/<id> for ShadowMountPlus), not from /data/homebrew: a new icon
# stayed the old one after a reboot (2026-10-17) until these were replaced.
PRESENTATION = ("icon0.png", "pic0.dds", "pic1.dds", "param.json")


def refresh_presentation(ftp: ftplib.FTP, local: pathlib.Path, title_id: str) -> int:
    count = 0
    targets = [f"/user/appmeta/{title_id}", f"/user/app/{title_id}/sce_sys"]
    for name in PRESENTATION:
        source = local / "sce_sys" / name
        if not source.is_file():
            continue
        places = [t + "/" + name for t in targets]
        if name == "icon0.png":
            places.append(f"/user/app/{title_id}/icon0.png")
        for target in places:
            try:
                with source.open("rb") as data:
                    ftp.storbinary(f"STOR {target}", data)
                chmod(ftp, target)
                count += 1
                print(f"  {target}")
            except ftplib.all_errors as error:
                print(f"  {target}: {error}")
    return count


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("host", help="PS5 IP address")
    parser.add_argument("--port", type=int, default=2121)
    parser.add_argument("--app", default="hub", help="hub (default) or app (the test app)")
    parser.add_argument("--log", action="store_true", help="fetch the app's log from /data/emuland")
    parser.add_argument("--folder", type=pathlib.Path,
                        help="send this app folder (a release's PPSA98265) instead of a build")
    args = parser.parse_args()

    param = (args.folder or ROOT / args.app) / "sce_sys" / "param.json"
    if not param.is_file():
        print(f"No {param}: is this an app folder?", file=sys.stderr)
        return 1
    title_id = json.loads(param.read_text(encoding="utf-8"))["titleId"]
    ftp = ftplib.FTP()
    ftp.connect(args.host, args.port, timeout=15)
    ftp.login()

    if args.log:
        out = ROOT / "logs"
        out.mkdir(exist_ok=True)
        names = ["hub.log", "hub.prev.log"] if args.app == "hub" else ["teste.log", "anterior.log"]
        for name in names:
            target = out / name
            try:
                with target.open("wb") as sink:
                    ftp.retrbinary(f"RETR /data/emuland/{name}", sink.write)
                print(f"Log saved to {target}")
            except ftplib.error_perm:
                target.unlink(missing_ok=True)
                print(f"No /data/emuland/{name} on the console")
        ftp.quit()
        return 0

    local = args.folder.resolve() if args.folder else ROOT / args.app / "dist" / title_id
    if not (local / "eboot.bin").is_file():
        print(f"No build in {local}; run scripts\\build-ps5.cmd first.", file=sys.stderr)
        return 1
    remote = f"/data/homebrew/{title_id}"
    print(f"Sending {local} to {args.host}:{remote}")
    count = upload(ftp, local, remote)
    if args.app == "hub" or args.folder:
        count += refresh_presentation(ftp, local, title_id)
    ftp.quit()
    print(f"Done: {count} files. Wait for ShadowMountPlus to show Manta, then open it.")
    return 0


if __name__ == "__main__":
    sys.exit(main())

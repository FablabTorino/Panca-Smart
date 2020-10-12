import subprocess


def get_hosts() -> int:
    _cmd = "chilli_query list | awk '{ if ($5 == 1) print $6 }' | wc -l"
    return int(subprocess.check_output(_cmd, shell=True).decode().strip())

cd /Users/stan/Workspace/framework2D
mkdir -p tmp
AUTO_SLOPE_TEST=1 \
AUTO_SLOPE_TELEMETRY=1 \
AUTO_SLOPE_LOG_PATH=tmp/auto_slope_telemetry.csv \
AUTO_INPUT_RECORD=1 \
AUTO_INPUT_RECORD_PATH=tmp/auto_input_events.csv \
./bin/framework2D_d --opengl

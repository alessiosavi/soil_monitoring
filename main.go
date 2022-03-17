package main

import (
	"fmt"
	"log"
	"math/rand"
	"net/http"
	"net/http/httputil"
	"strconv"
	"time"

	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/promhttp"
)

var (
	tempGauge = prometheus.NewGauge(
		prometheus.GaugeOpts{
			Namespace: "soil",
			Name:      "temperature",
			Help:      "Gauge of temperature",
		})

	humGauge = prometheus.NewGauge(
		prometheus.GaugeOpts{
			Namespace: "soil",
			Name:      "humidity",
			Help:      "Gauge of humidity",
		})
	lightGauge = prometheus.NewGauge(
		prometheus.GaugeOpts{
			Namespace: "soil",
			Name:      "light",
			Help:      "Gauge of light quantity",
		})

	soilGauge = prometheus.NewGauge(
		prometheus.GaugeOpts{
			Namespace: "soil",
			Name:      "soil",
			Help:      "Gauge of soil humidity",
		})

	lumenGauge = prometheus.NewGauge(
		prometheus.GaugeOpts{
			Namespace: "soil",
			Name:      "lumen",
			Help:      "Gauge of soil humidity",
		})

	heatGauge = prometheus.NewGauge(
		prometheus.GaugeOpts{
			Namespace: "soil",
			Name:      "heat",
			Help:      "Gauge of heat index",
		})
)

func main() {
	rand.Seed(time.Now().Unix())
	log.SetFlags(log.Lmicroseconds | log.LstdFlags | log.Lshortfile)
	log.Println("STARTING SOIL MONITORING ...")
	histogramVec := prometheus.NewHistogramVec(prometheus.HistogramOpts{
		Name: "prom_request_time",
		Help: "Time it has taken to retrieve the metrics",
	}, []string{"time"})

	if err := prometheus.Register(histogramVec); err != nil {
		panic(err)
	}

	http.HandleFunc("/status", func(writer http.ResponseWriter, request *http.Request) {
		writer.WriteHeader(202)
		writer.Write([]byte("UP"))
	})

	http.HandleFunc("/data", func(writer http.ResponseWriter, request *http.Request) {
		temp, okT := request.URL.Query()["temp"]
		hum, okH := request.URL.Query()["hum"]
		light, okL := request.URL.Query()["light"]
		soil, okS := request.URL.Query()["soil"]
		lumen, okLu := request.URL.Query()["lumen"]
		heat, okHeat := request.URL.Query()["heat"]

		if (!okT || len(temp) == 0) && (!okH || len(hum) == 0) && (!okL || len(light) == 0) && (!okS || len(soil) == 0) && (!okLu || len(lumen) == 0) && (!okHeat || len(heat) == 0) {
			writer.WriteHeader(http.StatusBadRequest)
			dumpRequest, err := httputil.DumpRequest(request, true)
			if err != nil {
				panic(err)
			}
			log.Println("Request without data: \n" + string(dumpRequest))
			return
		}

		if okT {
			fTemp, err := strconv.ParseFloat(temp[0], 64)
			if err != nil {
				writer.WriteHeader(http.StatusBadRequest)
				log.Println("ERROR! ", err)
				return
			}
			log.Printf("Adding new temperature data [%f]\n", fTemp)
			tempGauge.Set(fTemp)
		} else if okH {
			fHum, err := strconv.ParseFloat(hum[0], 64)
			if err != nil {
				writer.WriteHeader(http.StatusBadRequest)
				log.Println("ERROR! ", err)
			}
			log.Printf("Adding new humidity data [%f]\n", fHum)
			humGauge.Set(fHum)
		} else if okL {
			fLight, err := strconv.ParseFloat(light[0], 64)
			if err != nil {
				writer.WriteHeader(http.StatusBadRequest)
				log.Println("ERROR! ", err)
			}
			log.Printf("Adding new light data [%f]\n", fLight)
			lightGauge.Set(fLight)
		} else if okS {
			fSoil, err := strconv.ParseFloat(soil[0], 64)
			if err != nil {
				writer.WriteHeader(http.StatusBadRequest)
				log.Println("ERROR! ", err)
			}
			log.Printf("Adding new soil data [%f]\n", fSoil)
			soilGauge.Set(fSoil)
		} else if okLu {
			fLumen, err := strconv.ParseFloat(lumen[0], 64)
			if err != nil {
				writer.WriteHeader(http.StatusBadRequest)
				log.Println("ERROR! ", err)
			}
			log.Printf("Adding new lumen data [%f]\n", fLumen)
			lumenGauge.Set(fLumen)
		} else if okHeat {
			fHeat, err := strconv.ParseFloat(heat[0], 64)
			if err != nil {
				writer.WriteHeader(http.StatusBadRequest)
				log.Println("ERROR! ", err)
			}
			log.Printf("Adding new heat data [%f]\n", fHeat)
			heatGauge.Set(fHeat)
		}
	})
	http.Handle("/metrics", newHandlerWithHistogram(promhttp.Handler(), histogramVec))
	prometheus.MustRegister(tempGauge, humGauge, soilGauge, lightGauge, lumenGauge, heatGauge)
	log.Fatal(http.ListenAndServe("0.0.0.0:8080", nil))
}

func newHandlerWithHistogram(handler http.Handler, histogram *prometheus.HistogramVec) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, req *http.Request) {
		start := time.Now()
		status := http.StatusOK

		defer func() {
			histogram.WithLabelValues(fmt.Sprintf("%d", status)).Observe(time.Since(start).Seconds())
		}()

		if req.Method == http.MethodGet {
			handler.ServeHTTP(w, req)
			return
		}
		status = http.StatusBadRequest

		w.WriteHeader(status)
	})
}

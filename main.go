package main

import (
	"fmt"
	"log"
	"math/rand"
	"net/http"
	"net/http/httputil"
	strconv "strconv"
	"time"

	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/promhttp"
)

var (
	tempCounter = prometheus.NewGauge(
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
)

func main() {
	rand.Seed(time.Now().Unix())

	histogramVec := prometheus.NewHistogramVec(prometheus.HistogramOpts{
		Name: "prom_request_time",
		Help: "Time it has taken to retrieve the metrics",
	}, []string{"time"})

	if err := prometheus.Register(histogramVec); err != nil {
		panic(err)
	}

	http.HandleFunc("/data", func(writer http.ResponseWriter, request *http.Request) {
		temp, okT := request.URL.Query()["temp"]
		hum, okH := request.URL.Query()["hum"]
		if (!okT || len(temp) == 0) && (!okH || len(hum) == 0) {
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
				panic(err)
			}
			log.Printf("Adding new temperature data [%f]\n", fTemp)
			tempCounter.Set(fTemp)
		} else if okH {
			fHum, err := strconv.ParseFloat(hum[0], 64)
			if err != nil {
				writer.WriteHeader(http.StatusBadRequest)
				panic(err)
			}
			log.Printf("Adding new humidity data [%f]\n", fHum)
			humGauge.Set(fHum)
		}
	})
	http.Handle("/metrics", newHandlerWithHistogram(promhttp.Handler(), histogramVec))
	prometheus.MustRegister(tempCounter, humGauge)
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
